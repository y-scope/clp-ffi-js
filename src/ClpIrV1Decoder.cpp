

#include <emscripten/emscripten.h>
#include <emscripten/val.h>
#include <emscripten/bind.h>

#include <string>
#include <iostream>

#include "ffi/ir_stream/decoding_methods.hpp"
#include "spdlog_with_specializations.hpp"
#include "TimestampPattern.hpp"
#include "streaming_compression/zstd/Decompressor.hpp"
#include "ir/LogEventDeserializer.hpp"
#include "ffi/encoding_methods.hpp"

#ifdef __cplusplus
#define EXTERN extern "C"
#else
#define EXTERN
#endif

// FIXME: use macro
enum class LOG_VERBOSITY
{
    NONE = 0,
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL
};

const char *VERBOSITIES[] = {
    nullptr,
    "TRACE",
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "FATAL",
};

EM_JS(void, insert_row, (emscripten::EM_VAL results_handle, const char *message, int64_t timestamp, int verbosity), {
    const results = Emval.toValue(results_handle);
    results.push([ UTF8ToString(message), timestamp, verbosity ]);
});

class ClpIrV1Decoder
{
private:
    clp::streaming_compression::zstd::Decompressor m_zstd_decompressor;
    std::vector<clp::ir::LogEvent<clp::ir::four_byte_encoded_variable_t>> m_log_events;
    const int m_timezone_offset_ms{0 * 3600 * 1000};

    std::unique_ptr<clp::ir::LogEventDeserializer<clp::ir::four_byte_encoded_variable_t>> m_deserializer;
    clp::TimestampPattern m_ts_pattern;

public:
    ClpIrV1Decoder(const uintptr_t dataBuffer, const int length)
    {
        std::cout << "ClpIrV1Decoder::ClpIrV1Decoder() got buffer of length=" << length << std::endl;
        m_zstd_decompressor.open(reinterpret_cast<const char *>(dataBuffer), length);
    }

    int buildIdx()
    {
        decodePreamble();

        m_log_events.reserve(500000);
        int numDecoded = 0;
        while (true)
        {
            auto const result{m_deserializer->deserialize_log_event()};
            if (result.has_error())
            {
                auto const error{result.error()};
                if (std::errc::result_out_of_range == error)
                {
                    std::cerr << "File contains an incomplete IR stream" << std::endl;
                }
                else if (std::errc::no_message_available != error)
                {
                    std::cerr << "Failed to decompress: "
                              << error.category().name() << ":" << error.message()
                              << std::endl;
                }
                break;
            }

            numDecoded++;

            m_log_events.push_back(result.value());
        }

        return numDecoded;
    }

    bool decode(const emscripten::val &results, size_t startIdx, size_t endIdx)
    {
        for (size_t log_event_idx = startIdx; log_event_idx < endIdx; log_event_idx++)
        {
            auto const &log_event{m_log_events[log_event_idx]};
            std::string message;

            {
                auto constant_handler = [&](std::string const &value, size_t begin_pos, size_t length)
                {
                    message.append(value, begin_pos, length);
                };

                auto encoded_int_handler = [&](clp::ir::four_byte_encoded_variable_t value)
                { message.append(clp::ffi::decode_integer_var(value)); };

                auto encoded_float_handler = [&](clp::ir::four_byte_encoded_variable_t encoded_float)
                {
                    message.append(clp::ffi::decode_float_var(encoded_float));
                };

                auto dict_var_handler = [&](std::string const &dict_var)
                { message.append(dict_var); };

                try
                {
                    clp::ffi::ir_stream::generic_decode_message<true>(
                        log_event.get_logtype(),
                        log_event.get_encoded_vars(),
                        log_event.get_dict_vars(),
                        constant_handler,
                        encoded_int_handler,
                        encoded_float_handler,
                        dict_var_handler);
                }
                catch (clp::ffi::ir_stream::DecodingException const &e)
                {
                    std::cerr << "Failed to decode. Error Code: " << static_cast<uint32_t>(clp::ffi::ir_stream::IRErrorCode_Decode_Error)
                              << std::endl;

                    break;
                }
            }

            LOG_VERBOSITY verbosity_level = LOG_VERBOSITY::NONE;
            for (size_t verbosity_idx = 1; verbosity_idx < sizeof(VERBOSITIES) / sizeof(VERBOSITIES[0]); verbosity_idx++)
            {
                if (log_event.get_logtype().substr(1).starts_with(VERBOSITIES[verbosity_idx]))
                {
                    verbosity_level = static_cast<LOG_VERBOSITY>(verbosity_idx);
                    break;
                }
            }

            m_ts_pattern.insert_formatted_timestamp(
                log_event.get_timestamp() + m_timezone_offset_ms,
                message);

            insert_row(results.as_handle(), message.c_str(), log_event.get_timestamp(),
                       static_cast<int>(verbosity_level));
        }

        return true;
    }

private:
    bool decodePreamble()
    {
        bool is_four_bytes_encoding{true};

        if (auto const err{
                clp::ffi::ir_stream::get_encoding_type(m_zstd_decompressor, is_four_bytes_encoding)};
            clp::ffi::ir_stream::IRErrorCode::IRErrorCode_Success != err)
        {
            std::cout << "Failed to decode encoding type." << std::endl;
            return false;
        }
        if (!is_four_bytes_encoding)
        {
            std::cout << "Is not four byte encoding" << std::endl;
            return false;
        }
        auto result{
            clp::ir::LogEventDeserializer<clp::ir::four_byte_encoded_variable_t>::create(
                m_zstd_decompressor)};
        if (result.has_error())
        {
            auto const error_code{result.error()};

            std::cout << "Failed to decompress: "
                      << error_code.category().name() << ":" << error_code.message()
                      << std::endl;
            return false;
        }

        m_deserializer = std::make_unique<clp::ir::LogEventDeserializer<clp::ir::four_byte_encoded_variable_t>>(std::move(result.value()));
        m_ts_pattern = m_deserializer->get_timestamp_pattern();

        return true;
    }
};

EMSCRIPTEN_BINDINGS(DecoderModule)
{
    emscripten::class_<ClpIrV1Decoder>("ClpIrV1Decoder")
        .constructor<const uintptr_t, int>()
        .function("buildIdx", &ClpIrV1Decoder::buildIdx)
        .function("decode", &ClpIrV1Decoder::decode);
}