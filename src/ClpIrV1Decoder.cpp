#include "ClpIrV1Decoder.hpp"

#include <emscripten/emscripten.h>
#include <emscripten/val.h>

#include <clp/ffi/encoding_methods.hpp>
#include <clp/ffi/ir_stream/decoding_methods.hpp>
#include <clp/TimestampPattern.hpp>

#include "spdlog/spdlog.h"
#include "types.hpp"

static constexpr size_t cDefaultInitialNumLogEvents = 500'000;
static constexpr size_t cFullRangeEndIdx = 0;
static constexpr size_t cLogLevelNone = 0;

auto ClpIrV1Decoder::create(emscripten::val const& data_array) -> ClpIrV1Decoder* {
    auto length = data_array["length"].as<size_t>();
    SPDLOG_INFO("ClpIrV1Decoder::ClpIrV1Decoder() got buffer of length={}", length);

    auto data_buffer = std::vector<char const>(length);
    emscripten::val::module_property("HEAPU8")
            .call<void>("set", data_array, reinterpret_cast<uintptr_t>(data_buffer.data()));

    auto zstd_decompressor = std::make_shared<clp::streaming_compression::zstd::Decompressor>();
    zstd_decompressor->open(data_buffer.data(), length);

    bool is_four_bytes_encoding{true};
    if (auto const err{
                clp::ffi::ir_stream::get_encoding_type(*zstd_decompressor, is_four_bytes_encoding)
        };
        clp::ffi::ir_stream::IRErrorCode::IRErrorCode_Success != err)
    {
        SPDLOG_CRITICAL("Failed to decode encoding type.");
        throw err;
    }
    if (false == is_four_bytes_encoding) {
        SPDLOG_CRITICAL("Is not four byte encoding.");
        throw std::exception();
    }
    auto result{clp::ir::LogEventDeserializer<clp::ir::four_byte_encoded_variable_t>::create(
            *zstd_decompressor
    )};
    if (result.has_error()) {
        auto const error_code{result.error()};
        SPDLOG_CRITICAL(
                "Failed to decompress: {}:{}",
                error_code.category().name(),
                error_code.message()
        );
        throw std::exception();
    }

    return new ClpIrV1Decoder(std::move(data_buffer), zstd_decompressor, std::move(result.value()));
}

ClpIrV1Decoder::ClpIrV1Decoder(
        std::vector<char const> data_buffer,
        std::shared_ptr<clp::streaming_compression::zstd::Decompressor> zstd_decompressor,
        clp::ir::LogEventDeserializer<clp::ir::four_byte_encoded_variable_t> deserializer
)
        : m_data_buffer{std::move(data_buffer)},
          m_zstd_decompressor{zstd_decompressor},
          m_deserializer{std::move(deserializer)} {
    m_ts_pattern = m_deserializer.get_timestamp_pattern();
}

ClpIrV1Decoder::~ClpIrV1Decoder() {}

size_t ClpIrV1Decoder::get_estimated_num_events() {
    return m_log_events.size();
}

emscripten::val ClpIrV1Decoder::build_idx(size_t begin_idx, size_t end_idx) {
    emscripten::val results = emscripten::val::object();
    if (cFullRangeEndIdx != end_idx) {
        SPDLOG_ERROR("Partial range indexing building is not yet supported.");
        results.set("numValidEvents", 0);
        results.set("numInvalidEvents", end_idx - begin_idx);
        return results;
    } else if (m_log_events.empty()) {
        m_log_events.reserve(cDefaultInitialNumLogEvents);
        while (true) {
            auto const result{m_deserializer.deserialize_log_event()};
            if (result.has_error()) {
                auto const error{result.error()};
                if (std::errc::result_out_of_range == error) {
                    SPDLOG_ERROR("File contains an incomplete IR stream");
                } else if (std::errc::no_message_available != error) {
                    SPDLOG_ERROR(
                            "Failed to decompress: {}:{}",
                            error.category().name(),
                            error.message()
                    );
                }
                break;
            }
            m_log_events.emplace_back(result.value());
        }
        m_data_buffer.clear();
    }

    results.set("numValidEvents", m_log_events.size());
    results.set("numInvalidEvents", 0);
    return results;
}

emscripten::val ClpIrV1Decoder::decode(size_t begin_idx, size_t end_idx) {
    if (m_log_events.size() < end_idx) {
        return emscripten::val::null();
    }

    emscripten::val results = emscripten::val::array();
    std::span<clp::ir::LogEvent<clp::ir::four_byte_encoded_variable_t> const> log_events_span(
            m_log_events.data() + begin_idx,
            end_idx - begin_idx
    );
    for (auto const& log_event : log_events_span) {
        std::string message;

        auto constant_handler = [&](std::string const& value, size_t begin_pos, size_t length) {
            message.append(value, begin_pos, length);
        };

        auto encoded_int_handler = [&](clp::ir::four_byte_encoded_variable_t value) {
            message.append(clp::ffi::decode_integer_var(value));
        };

        auto encoded_float_handler = [&](clp::ir::four_byte_encoded_variable_t encoded_float) {
            message.append(clp::ffi::decode_float_var(encoded_float));
        };

        auto dict_var_handler = [&](std::string const& dict_var) { message.append(dict_var); };

        try {
            clp::ffi::ir_stream::generic_decode_message<true>(
                    log_event.get_logtype(),
                    log_event.get_encoded_vars(),
                    log_event.get_dict_vars(),
                    constant_handler,
                    encoded_int_handler,
                    encoded_float_handler,
                    dict_var_handler
            );
        } catch (clp::ffi::ir_stream::DecodingException const& e) {
            SPDLOG_ERROR("Failed to decode. Error Code: {}", e.get_error_code());
            break;
        }

        size_t log_level = cLogLevelNone;
        for (auto const& log_level_name : cLogLevelNames) {
            if (message.substr(1).starts_with(log_level_name)) {
                log_level = &log_level_name - cLogLevelNames.data();
                break;
            }
        }

        m_ts_pattern.insert_formatted_timestamp(log_event.get_timestamp(), message);

        EM_ASM(
                { Emval.toValue($0).push([UTF8ToString($1), $2, $3, $4]); },
                results.as_handle(),
                message.c_str(),
                log_event.get_timestamp(),
                log_level,
                begin_idx + (&log_event - log_events_span.data()) + 1
        );
    }

    return results;
}
