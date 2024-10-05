#include "StreamReader.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include <clp/Array.hpp>
#include <clp/ErrorCode.hpp>
#include <clp/ffi/ir_stream/decoding_methods.hpp>
#include <clp/ir/LogEventDeserializer.hpp>
#include <clp/ir/types.hpp>
#include <clp/streaming_compression/zstd/Decompressor.hpp>
#include <clp/TraceableException.hpp>
#include <emscripten/em_asm.h>
#include <emscripten/val.h>
#include <spdlog/spdlog.h>

#include <clp_ffi_js/ClpFfiJsException.hpp>
#include <clp_ffi_js/constants.hpp>
#include <clp_ffi_js/ir/LogEventWithLevel.hpp>
#include <clp_ffi_js/ir/StreamReaderDataContext.hpp>

using namespace std::literals::string_literals;
using clp::ir::four_byte_encoded_variable_t;

namespace clp_ffi_js::ir {
auto StreamReader::create(DataArrayTsType const& data_array) -> StreamReader {
    auto const length{data_array["length"].as<size_t>()};
    SPDLOG_INFO("StreamReader::create: got buffer of length={}", length);

    // Copy array from JavaScript to C++
    clp::Array<char> data_buffer{length};
    // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
    emscripten::val::module_property("HEAPU8")
            .call<void>("set", data_array, reinterpret_cast<uintptr_t>(data_buffer.data()));
    // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)

    auto zstd_decompressor{std::make_unique<clp::streaming_compression::zstd::Decompressor>()};
    zstd_decompressor->open(data_buffer.data(), length);

    bool is_four_bytes_encoding{true};
    if (auto const err{
                clp::ffi::ir_stream::get_encoding_type(*zstd_decompressor, is_four_bytes_encoding)
        };
        clp::ffi::ir_stream::IRErrorCode::IRErrorCode_Success != err)
    {
        SPDLOG_CRITICAL("Failed to decode encoding type, err={}", err);
        throw ClpFfiJsException{
                clp::ErrorCode::ErrorCode_MetadataCorrupted,
                __FILENAME__,
                __LINE__,
                "Failed to decode encoding type."
        };
    }
    if (false == is_four_bytes_encoding) {
        throw ClpFfiJsException{
                clp::ErrorCode::ErrorCode_Unsupported,
                __FILENAME__,
                __LINE__,
                "IR stream uses unsupported encoding."
        };
    }

    auto result{
            clp::ir::LogEventDeserializer<four_byte_encoded_variable_t>::create(*zstd_decompressor)
    };
    if (result.has_error()) {
        auto const error_code{result.error()};
        SPDLOG_CRITICAL(
                "Failed to create deserializer: {}:{}",
                error_code.category().name(),
                error_code.message()
        );
        throw ClpFfiJsException{
                clp::ErrorCode::ErrorCode_Failure,
                __FILENAME__,
                __LINE__,
                "Failed to create deserializer"
        };
    }

    StreamReaderDataContext<four_byte_encoded_variable_t> stream_reader_data_context{
            std::move(data_buffer),
            std::move(zstd_decompressor),
            std::move(result.value())
    };
    return StreamReader{std::move(stream_reader_data_context)};
}

auto StreamReader::get_num_events_buffered() const -> size_t {
    return m_encoded_log_events.size();
}

auto StreamReader::get_filtered_log_event_map() const -> FilteredLogEventMapType {
    if (std::nullopt == m_filtered_log_event_map) {
        return FilteredLogEventMapType(emscripten::val::null());
    }

    return FilteredLogEventMapType(emscripten::val::array(m_filtered_log_event_map.value()));
}

auto StreamReader::build() -> size_t {
    if (nullptr != m_stream_reader_data_context) {
        constexpr size_t cDefaultNumReservedLogEvents{500'000};
        m_encoded_log_events.reserve(cDefaultNumReservedLogEvents);

        std::string logtype;
        constexpr size_t cDefaultReservedMessageLength{512};
        logtype.reserve(cDefaultReservedMessageLength);
        while (true) {
            auto result{m_stream_reader_data_context->get_deserializer().deserialize_log_event()};
            if (false == result.has_error()) {
                auto const log_event = result.value();
                auto const message = log_event.get_message();

                logtype.clear();
                logtype = message.get_logtype();

                constexpr size_t cLogLevelPositionInMessages{1};
                if (logtype.length() < cLogLevelPositionInMessages) {
                    SPDLOG_WARN("Message is too short to extract log level.");
                    break;
                }

                size_t log_level{cLogLevelNone};
                // NOLINTNEXTLINE(readability-qualified-auto)
                auto const log_level_name_it{std::find_if(
                        cLogLevelNames.begin() + cValidLogLevelsBeginIdx,
                        cLogLevelNames.end(),
                        [&](std::string_view level) {
                            return logtype.substr(cLogLevelPositionInMessages).starts_with(level);
                        }
                )};
                if (log_level_name_it != cLogLevelNames.end()) {
                    log_level = std::distance(cLogLevelNames.begin(), log_level_name_it);
                }

                auto const log_viewer_event = LogEventWithLevel<four_byte_encoded_variable_t>(
                        log_event.get_timestamp(),
                        log_event.get_utc_offset(),
                        message,
                        log_level
                );

                m_encoded_log_events.emplace_back(std::move(log_viewer_event));
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
            throw ClpFfiJsException{
                    clp::ErrorCode::ErrorCode_Corrupt,
                    __FILENAME__,
                    __LINE__,
                    "Failed to deserialize: "s + error.category().name() + ":" + error.message()
            };
        }
        m_stream_reader_data_context.reset(nullptr);
    }
    return m_encoded_log_events.size();
}

auto StreamReader::decode_range(size_t begin_idx, size_t end_idx, bool use_filter) const
        -> DecodedResultsTsType {
    if (use_filter && (std::nullopt == m_filtered_log_event_map)) {
        return DecodedResultsTsType(emscripten::val::null());
    }

    size_t length;
    if (use_filter) {
        length = m_filtered_log_event_map->size();
    } else {
        length = m_encoded_log_events.size();
    }

    if (length < end_idx || 0 > begin_idx) {
        return DecodedResultsTsType(emscripten::val::null());
    }
    std::string message;
    constexpr size_t cDefaultReservedMessageLength{512};
    message.reserve(cDefaultReservedMessageLength);
    auto const results{emscripten::val::array()};

    for (size_t i = begin_idx; i < end_idx; ++i) {
        size_t log_event_idx;
        if (use_filter) {
            log_event_idx = m_filtered_log_event_map->at(i);
        } else {
            log_event_idx = i;
        }
        auto const& log_event = m_encoded_log_events[log_event_idx];
        message.clear();

        auto const parsed{log_event.get_message().decode_and_unparse()};
        if (false == parsed.has_value()) {
            SPDLOG_ERROR("Failed to decode message.");
            break;
        }
        message.append(parsed.value());

        m_ts_pattern.insert_formatted_timestamp(log_event.get_timestamp(), message);

        EM_ASM(
                { Emval.toValue($0).push([UTF8ToString($1), $2, $3, $4]); },
                results.as_handle(),
                message.c_str(),
                log_event.get_timestamp(),
                log_event.get_log_level(),
                log_event_idx + 1
        );
    }

    return DecodedResultsTsType(results);
}

void StreamReader::filter_log_events(emscripten::val const& logLevelFilter) {
    if (logLevelFilter.isNull()) {
        m_filtered_log_event_map.reset();
        return;
    }

    m_filtered_log_event_map.emplace();
    std::vector<int> filter_levels = emscripten::vecFromJSArray<int>(logLevelFilter);

    for (auto const& [logEventIdx, logEvent] : std::views::enumerate(m_encoded_log_events)) {
        if (std::ranges::find(filter_levels, logEvent.get_log_level()) != filter_levels.end()) {
            m_filtered_log_event_map->push_back(logEventIdx);
        }
    }
}

StreamReader::StreamReader(
        StreamReaderDataContext<four_byte_encoded_variable_t>&& stream_reader_data_context
)
        : m_stream_reader_data_context{std::make_unique<
                  StreamReaderDataContext<four_byte_encoded_variable_t>>(
                  std::move(stream_reader_data_context)
          )},
          m_ts_pattern{m_stream_reader_data_context->get_deserializer().get_timestamp_pattern()} {}
}  // namespace clp_ffi_js::ir

namespace {
EMSCRIPTEN_BINDINGS(ClpIrStreamReader) {
    emscripten::register_type<clp_ffi_js::ir::DataArrayTsType>("Uint8Array");
    emscripten::register_type<clp_ffi_js::ir::DecodedResultsTsType>(
            "Array<[string, number, number, number]>"
    );
    emscripten::register_type<clp_ffi_js::ir::FilteredLogEventMapType>("number[]");

    emscripten::class_<clp_ffi_js::ir::StreamReader>("ClpIrStreamReader")
            .constructor(
                    &clp_ffi_js::ir::StreamReader::create,
                    emscripten::return_value_policy::take_ownership()
            )
            .function(
                    "getNumEventsBuffered",
                    &clp_ffi_js::ir::StreamReader::get_num_events_buffered
            )
            .function(
                    "getFilteredLogEventMap",
                    &clp_ffi_js::ir::StreamReader::get_filtered_log_event_map
            )
            .function("build", &clp_ffi_js::ir::StreamReader::build)
            .function("decodeRange", &clp_ffi_js::ir::StreamReader::decode_range)
            .function("filterLogEvents", &clp_ffi_js::ir::StreamReader::filter_log_events);
}
}  // namespace
