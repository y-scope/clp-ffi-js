#include "StreamReader.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <system_error>
#include <utility>

#include <clp/ErrorCode.hpp>
#include <clp/ffi/encoding_methods.hpp>
#include <clp/ffi/ir_stream/decoding_methods.hpp>
#include <clp/ir/LogEvent.hpp>
#include <clp/ir/LogEventDeserializer.hpp>
#include <clp/ir/types.hpp>
#include <clp/streaming_compression/zstd/Decompressor.hpp>
#include <clp/TraceableException.hpp>
#include <emscripten/em_asm.h>
#include <emscripten/val.h>
#include <spdlog/spdlog.h>

#include <clp_ffi_js/ClpJsException.hpp>
#include <clp_ffi_js/constants.hpp>

using namespace std::literals::string_literals;

namespace clp_ffi_js::ir {
auto StreamReader::create(emscripten::val const& data_array) -> StreamReader {
    auto const length{data_array["length"].as<size_t>()};
    SPDLOG_INFO("StreamReader::StreamReader() got buffer of length={}", length);

    // Copy array from JavaScript to C++
    std::vector<char> data_buffer(length);
    // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
    emscripten::val::module_property("HEAPU8")
            .call<void>("set", data_array, reinterpret_cast<uintptr_t>(data_buffer.data()));
    // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)

    auto const zstd_decompressor{std::make_shared<clp::streaming_compression::zstd::Decompressor>()
    };
    zstd_decompressor->open(data_buffer.data(), length);

    bool is_four_bytes_encoding{true};
    if (auto const err{
                clp::ffi::ir_stream::get_encoding_type(*zstd_decompressor, is_four_bytes_encoding)
        };
        clp::ffi::ir_stream::IRErrorCode::IRErrorCode_Success != err)
    {
        SPDLOG_CRITICAL("Failed to decode encoding type, err={}", err);
        throw ClpJsException(
                clp::ErrorCode::ErrorCode_MetadataCorrupted,
                __FILENAME__,
                __LINE__,
                "Failed to decode encoding type."
        );
    }
    if (false == is_four_bytes_encoding) {
        throw ClpJsException(
                clp::ErrorCode::ErrorCode_Unsupported,
                __FILENAME__,
                __LINE__,
                "IR stream uses unsupported encoding."
        );
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
        throw ClpJsException(
                clp::ErrorCode::ErrorCode_Failure,
                __FILENAME__,
                __LINE__,
                "Failed to create deserializer"
        );
    }

    return StreamReader(std::move(data_buffer), zstd_decompressor, std::move(result.value()));
}

auto StreamReader::get_num_events_buffered() const -> size_t {
    return m_log_events.size();
}

auto StreamReader::build_idx(size_t begin_idx, size_t end_idx) -> size_t {
    constexpr size_t cFullRangeEndIdx{0};
    if (0 != begin_idx || cFullRangeEndIdx != end_idx) {
        throw ClpJsException(
                clp::ErrorCode::ErrorCode_Unsupported,
                __FILENAME__,
                __LINE__,
                "Partial range index building is not yet supported."
        );
    }
    if (false == m_read_complete) {
        m_read_complete = true;
        constexpr size_t cDefaultNumLogEvents{500'000};
        m_log_events.reserve(cDefaultNumLogEvents);
        while (true) {
            auto result{m_deserializer.deserialize_log_event()};
            if (false == result.has_error()) {
                m_log_events.emplace_back(std::move(result.value()));
                continue;
            }
            auto const error{result.error()};
            if (std::errc::no_message_available == error) {
                break;
            }
            if (std::errc::result_out_of_range == error) {
                SPDLOG_ERROR("File contains an incomplete IR stream");
                break;
            }
            throw ClpJsException(
                    clp::ErrorCode::ErrorCode_Corrupt,
                    __FILENAME__,
                    __LINE__,
                    "Failed to decompress: "s + error.category().name() + ":" + error.message()
            );
        }
        m_data_buffer.reset(nullptr);
        m_zstd_decompressor->close();
    }

    return m_log_events.size();
}

auto StreamReader::decode(size_t begin_idx, size_t end_idx) const -> emscripten::val {
    if (m_log_events.size() < end_idx || begin_idx >= end_idx) {
        return emscripten::val::null();
    }

    auto log_events_span = std::span{
            m_log_events.begin() + static_cast<decltype(m_log_events)::difference_type>(begin_idx),
            m_log_events.begin() + static_cast<decltype(m_log_events)::difference_type>(end_idx)
    };
    std::string message;
    constexpr size_t cDefaultReservedMessageLength{512};
    message.reserve(cDefaultReservedMessageLength);
    size_t log_num{begin_idx + 1};
    auto const results{emscripten::val::array()};
    for (auto const& log_event : log_events_span) {
        message.clear();

        auto const parsed{log_event.get_message().decode_and_unparse()};
        if (false == parsed.has_value()) {
            SPDLOG_ERROR("Failed to decode message.");
            break;
        } else {
            message.append(parsed.value());
        }

        constexpr size_t cLogLevelPositionInMessages{1};
        size_t log_level{cLogLevelNone};
        auto const log_level_name_it = std::find_if(
            cLogLevelNames.begin() + cValidLogLevelsBeginIdx, cLogLevelNames.end(),
            [&](std::string_view level) {
                return message.substr(cLogLevelPositionInMessages).starts_with(level);
            });
        if (log_level_name_it != cLogLevelNames.end()) {
            log_level = std::distance(cLogLevelNames.begin(), log_level_name_it);
        }

        m_ts_pattern.insert_formatted_timestamp(log_event.get_timestamp(), message);

        EM_ASM(
                { Emval.toValue($0).push([UTF8ToString($1), $2, $3, $4]); },
                results.as_handle(),
                message.c_str(),
                log_event.get_timestamp(),
                log_level,
                log_num
        );
        ++log_num;
    }

    return results;
}

StreamReader::StreamReader(
        std::vector<char>&& data_buffer,
        std::shared_ptr<clp::streaming_compression::zstd::Decompressor> zstd_decompressor,
        clp::ir::LogEventDeserializer<clp::ir::four_byte_encoded_variable_t> deserializer
)
        : m_data_buffer{std::make_unique<std::vector<char>>(std::move(data_buffer))},
          m_zstd_decompressor{std::move(zstd_decompressor)},
          m_deserializer{std::move(deserializer)},
          m_ts_pattern{m_deserializer.get_timestamp_pattern()} {}
}  // namespace clp_ffi_js::ir

namespace {
EMSCRIPTEN_BINDINGS(ClpIrStreamReader) {
    emscripten::class_<clp_ffi_js::ir::StreamReader>("ClpIrStreamReader")
            .constructor(
                    &clp_ffi_js::ir::StreamReader::create,
                    emscripten::return_value_policy::take_ownership()
            )
            .function(
                    "estimatedNumEvents",
                    &clp_ffi_js::ir::StreamReader::get_num_events_buffered
            )
            .function("buildIdx", &clp_ffi_js::ir::StreamReader::build_idx)
            .function("decode", &clp_ffi_js::ir::StreamReader::decode);
}
}  // namespace
