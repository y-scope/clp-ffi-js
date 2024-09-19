#include "StreamReader.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <numeric>
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
#include <clp_ffi_js/ir/StreamReaderDataContext.hpp>
#include <clp_ffi_js/ir/LogViewerEvent.hpp>


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

auto StreamReader::deserialize_range(size_t begin_idx, size_t end_idx) -> size_t {
    constexpr size_t cFullRangeEndIdx{0};
    if (0 != begin_idx || cFullRangeEndIdx != end_idx) {
        throw ClpFfiJsException{
                clp::ErrorCode::ErrorCode_Unsupported,
                __FILENAME__,
                __LINE__,
                "Partial range deserialization is not yet supported."
        };
    }
    if (nullptr != m_stream_reader_data_context) {
        constexpr size_t cDefaultNumReservedLogEvents{500'000};
        m_encoded_log_events.reserve(cDefaultNumReservedLogEvents);
        while (true) {
            auto result{m_stream_reader_data_context->get_deserializer().deserialize_log_event()};
            if (false == result.has_error()) {

                const auto log_event = result.value();
                const auto message = log_event.get_message();

                std::string logtype;
                constexpr size_t cDefaultReservedMessageLength{512};
                logtype.reserve(cDefaultReservedMessageLength);
                logtype = message.get_logtype();


                constexpr size_t cLogLevelPositionInMessages{1};
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
                const auto log_viewer_event = clp::ffi::js::LogViewerEvent<int>(log_event.get_timestamp(), log_event.get_utc_offset(), log_event.get_message(),log_level);

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
    m_filtered_log_event_indices = clp_ffi_js::ir::StreamReader::create_indices_vector(m_encoded_log_events.size());
    return m_encoded_log_events.size();
}

auto StreamReader::decode_range(size_t begin_idx, size_t end_idx) const -> DecodedResultsTsType {
    if (m_filtered_log_event_indices.size() < end_idx || 0 > begin_idx) {
        return DecodedResultsTsType(emscripten::val::null());
    }
    std::string message;
    constexpr size_t cDefaultReservedMessageLength{512};
    message.reserve(cDefaultReservedMessageLength);
    size_t log_num{begin_idx + 1};
    auto const results{emscripten::val::array()};

    for (size_t filtered_log_event_idx = begin_idx; filtered_log_event_idx < end_idx; ++filtered_log_event_idx) {
         // Get the log event index from filtered indices
        size_t log_event_idx = m_filtered_log_event_indices[filtered_log_event_idx];
        const auto& log_event = m_encoded_log_events[log_event_idx];
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
                log_num
        );
        ++log_num;
    }

    return DecodedResultsTsType(results);
}

void StreamReader::filter_logs(const emscripten::val& logLevelFilter) {
    // Clear the previous filtered log indices
    m_filtered_log_event_indices.clear();

    // Check if the filter is empty
    if (logLevelFilter.isNull() || logLevelFilter["length"].as<int>() == 0) {
        m_filtered_log_event_indices = create_indices_vector(m_encoded_log_events.size());
        return;
    }

    // Convert JavaScript array to C++ vector
    std::vector<int> filter_levels;
    unsigned int length = logLevelFilter["length"].as<unsigned int>();
    filter_levels.reserve(length);
    for (unsigned int i = 0; i < length; ++i) {
        filter_levels.push_back(logLevelFilter[i].as<int>());
    }

    // Filter log events based on the provided log levels
    for (size_t index = 0; index < m_encoded_log_events.size(); ++index) {
        const auto& logEvent = m_encoded_log_events[index];
        if (std::find(filter_levels.begin(), filter_levels.end(), logEvent.get_log_level()) != filter_levels.end()) {
            m_filtered_log_event_indices.push_back(index);
        }
    }
}

    auto StreamReader::get_filtered_log_indices() const ->  FilterIndicesType {
        // Convert C++ vector to JavaScript array using EM_ASM
        emscripten::val js_array = emscripten::val::array();
        for (size_t index : m_filtered_log_event_indices) {
            EM_ASM_(
                {
                    Emval.toValue($0).push($1);
                },
                js_array.as_handle(),
                index
            );
        }
        return FilterIndicesType(js_array);
    }


auto StreamReader::create_indices_vector(int length) -> std::vector<size_t> {
    std::vector<size_t> indices(length);
    std::iota(indices.begin(), indices.end(), 0); // Fill with 0, 1, 2, ...
    return indices;
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
    emscripten::register_type<clp_ffi_js::ir::FilterIndicesType>(
            "Array<[number]>"
    );

    emscripten::register_vector<size_t>("VectorSizeT");

    emscripten::class_<clp_ffi_js::ir::StreamReader>("ClpIrStreamReader")
            .constructor(
                    &clp_ffi_js::ir::StreamReader::create,
                    emscripten::return_value_policy::take_ownership()
            )
            .function(
                    "getNumEventsBuffered",
                    &clp_ffi_js::ir::StreamReader::get_num_events_buffered
            )
            .function("deserializeRange", &clp_ffi_js::ir::StreamReader::deserialize_range)
            .function("decodeRange", &clp_ffi_js::ir::StreamReader::decode_range)
            .function("filter_logs", &clp_ffi_js::ir::StreamReader::filter_logs)
            .function("get_filtered_log_indices", &clp_ffi_js::ir::StreamReader::get_filtered_log_indices);
}
}  // namespace
