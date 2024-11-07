#include "StructuredIrStreamReader.hpp"

#include <cstddef>
#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include <clp/Array.hpp>
#include <clp/ErrorCode.hpp>
#include <clp/ffi/ir_stream/Deserializer.hpp>
#include <clp/ffi/KeyValuePairLogEvent.hpp>
#include <clp/ffi/Value.hpp>
#include <clp/ir/types.hpp>
#include <clp/TraceableException.hpp>
#include <emscripten/em_asm.h>
#include <emscripten/val.h>
#include <spdlog/spdlog.h>

#include <clp_ffi_js/ClpFfiJsException.hpp>
#include <clp_ffi_js/constants.hpp>
#include <clp_ffi_js/ir/StreamReader.hpp>
#include <clp_ffi_js/ir/StreamReaderDataContext.hpp>

namespace clp_ffi_js::ir {
using clp::ir::four_byte_encoded_variable_t;

static constexpr std::string_view cLogLevelFilteringNotSupportedPrompt{
        "Log level filtering is not yet supported in this reader."
};

auto StructuredIrStreamReader::create(
        std::unique_ptr<ZstdDecompressor>&& zstd_decompressor,
        clp::Array<char> data_array,
        ReaderOptions const& reader_options
) -> StructuredIrStreamReader {
    auto deserialized_log_events{std::make_shared<std::vector<clp::ffi::KeyValuePairLogEvent>>()};
    auto result{StructuredIrDeserializer::create(
            *zstd_decompressor,
            IrUnitHandler{
                    deserialized_log_events,
                    reader_options["logLevelKey"].as<std::string>(),
                    reader_options["timestampKey"].as<std::string>()
            }
    )};
    if (result.has_error()) {
        auto const error_code{result.error()};
        throw ClpFfiJsException{
                clp::ErrorCode::ErrorCode_Failure,
                __FILENAME__,
                __LINE__,
                std::format(
                        "Failed to create deserializer: {} {}",
                        error_code.category().name(),
                        error_code.message()
                )
        };
    }
    StreamReaderDataContext<StructuredIrDeserializer> data_context{
            std::move(data_array),
            std::move(zstd_decompressor),
            std::move(result.value())
    };
    return StructuredIrStreamReader{std::move(data_context), std::move(deserialized_log_events)};
}

auto StructuredIrStreamReader::get_num_events_buffered() const -> size_t {
    return m_deserialized_log_events->size();
}

auto StructuredIrStreamReader::get_filtered_log_event_map() const -> FilteredLogEventMapTsType {
    SPDLOG_ERROR(cLogLevelFilteringNotSupportedPrompt);
    return FilteredLogEventMapTsType{emscripten::val::null()};
}

void StructuredIrStreamReader::filter_log_events(LogLevelFilterTsType const& log_level_filter) {
    if (log_level_filter.isNull()) {
        return;
    }
    SPDLOG_ERROR(cLogLevelFilteringNotSupportedPrompt);
}

auto StructuredIrStreamReader::deserialize_stream() -> size_t {
    if (nullptr == m_stream_reader_data_context) {
        return m_deserialized_log_events->size();
    }

    constexpr size_t cDefaultNumReservedLogEvents{500'000};
    m_deserialized_log_events->reserve(cDefaultNumReservedLogEvents);
    auto& reader{m_stream_reader_data_context->get_reader()};
    while (true) {
        auto result{m_stream_reader_data_context->get_deserializer().deserialize_next_ir_unit(reader
        )};
        if (false == result.has_error()) {
            continue;
        }
        auto const error{result.error()};
        if (std::errc::no_message_available == error || std::errc::operation_not_permitted == error)
        {
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
                std::format(
                        "Failed to deserialize IR unit: {}:{}",
                        error.category().name(),
                        error.message()
                )
        };
    }
    m_level_node_id = m_stream_reader_data_context->get_deserializer()
                              .get_ir_unit_handler()
                              .get_level_node_id();
    m_timestamp_node_id = m_stream_reader_data_context->get_deserializer()
                                  .get_ir_unit_handler()
                                  .get_timestamp_node_id();
    m_stream_reader_data_context.reset(nullptr);
    return m_deserialized_log_events->size();
}

auto StructuredIrStreamReader::decode_range(size_t begin_idx, size_t end_idx, bool use_filter) const
        -> DecodedResultsTsType {
    if (use_filter) {
        SPDLOG_ERROR(cLogLevelFilteringNotSupportedPrompt);
        return DecodedResultsTsType{emscripten::val::null()};
    }

    if (m_deserialized_log_events->size() < end_idx || begin_idx > end_idx) {
        return DecodedResultsTsType{emscripten::val::null()};
    }

    std::string message;
    constexpr size_t cDefaultReservedMessageLength{512};
    message.reserve(cDefaultReservedMessageLength);
    auto const results{emscripten::val::array()};

    for (size_t log_event_idx = begin_idx; log_event_idx < end_idx; ++log_event_idx) {
        auto const& log_event{m_deserialized_log_events->at(log_event_idx)};

        auto const json{log_event.serialize_to_json()};
        if (false == json.has_value()) {
            SPDLOG_ERROR("Failed to decode log event.");
            break;
        }

        auto const& id_value_pairs{log_event.get_node_id_value_pairs()};
        LogLevel log_level{LogLevel::NONE};
        if (m_level_node_id.has_value()) {
            auto const& log_level_pair{id_value_pairs.at(m_level_node_id.value())};
            log_level = log_level_pair.has_value()
                                ? static_cast<LogLevel>(
                                          log_level_pair.value()
                                                  .get_immutable_view<clp::ffi::value_int_t>()
                                  )
                                : log_level;
        }
        clp::ffi::value_int_t timestamp{0};
        if (m_timestamp_node_id.has_value()) {
            auto const& timestamp_pair{id_value_pairs.at(m_timestamp_node_id.value())};
            timestamp = timestamp_pair.has_value()
                                ? timestamp_pair.value().get_immutable_view<clp::ffi::value_int_t>()
                                : timestamp;
        }

        EM_ASM(
                { Emval.toValue($0).push([UTF8ToString($1), $2, $3, $4]); },
                results.as_handle(),
                json.value().dump().c_str(),
                timestamp,
                log_level,
                log_event_idx + 1
        );
    }

    return DecodedResultsTsType(results);
}

StructuredIrStreamReader::StructuredIrStreamReader(
        StreamReaderDataContext<StructuredIrDeserializer>&& stream_reader_data_context,
        std::shared_ptr<std::vector<clp::ffi::KeyValuePairLogEvent>> deserialized_log_events
)
        : m_stream_reader_data_context{std::make_unique<
                  StreamReaderDataContext<StructuredIrDeserializer>>(
                  std::move(stream_reader_data_context)
          )},
          m_deserialized_log_events{std::move(deserialized_log_events)} {}
}  // namespace clp_ffi_js::ir
