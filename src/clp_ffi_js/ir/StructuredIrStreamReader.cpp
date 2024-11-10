#include "StructuredIrStreamReader.hpp"

#include <cstddef>
#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <type_utils.hpp>
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
#include <clp_ffi_js/ir/LogEventWithFilterData.hpp>
#include <clp_ffi_js/ir/StreamReader.hpp>
#include <clp_ffi_js/ir/StreamReaderDataContext.hpp>

namespace clp_ffi_js::ir {
namespace {
constexpr std::string_view cEmptyJsonStr{"{}"};
constexpr std::string_view cLogLevelFilteringNotSupportedErrorMsg{
        "Log level filtering is not yet supported in this reader."
};
constexpr std::string_view cReaderOptionsLogLevelKey{"logLevelKey"};
constexpr std::string_view cReaderOptionsTimestampKey{"timestampKey"};

/**
 * Gets the `LogLevel` from an input string.
 * @param str
 * @return
 */
auto get_log_level(std::string_view str) -> LogLevel;

auto get_log_level(std::string_view str) -> LogLevel {
    LogLevel log_level{LogLevel::NONE};

    // Convert the string to uppercase,
    std::string log_level_name_upper_case{str};
    std::transform(
            log_level_name_upper_case.begin(),
            log_level_name_upper_case.end(),
            log_level_name_upper_case.begin(),
            [](unsigned char c) { return std::toupper(c); }
    );

    // Do not accept "None" when checking if input string is in `cLogLevelNames`.
    auto const* it = std::ranges::find(
            cLogLevelNames.begin() + clp::enum_to_underlying_type(cValidLogLevelsBeginIdx),
            cLogLevelNames.end(),
            log_level_name_upper_case
    );

    if (it == cLogLevelNames.end()) {
        return log_level;
    }

    return static_cast<LogLevel>(std::distance(cLogLevelNames.begin(), it));
    ;
}

}  // namespace

using clp::ir::four_byte_encoded_variable_t;

auto IrUnitHandler::handle_schema_tree_node_insertion(
        clp::ffi::SchemaTree::NodeLocator schema_tree_node_locator
) -> clp::ffi::ir_stream::IRErrorCode {
    ++m_current_node_id;

    auto const& key_name{schema_tree_node_locator.get_key_name()};
    if (m_log_level_key == key_name) {
        m_log_level_node_id.emplace(m_current_node_id);
    } else if (m_timestamp_key == key_name) {
        m_timestamp_node_id.emplace(m_current_node_id);
    }

    return clp::ffi::ir_stream::IRErrorCode::IRErrorCode_Success;
}

auto IrUnitHandler::handle_log_event(StructuredLogEvent&& log_event
) -> clp::ffi::ir_stream::IRErrorCode {
    auto const& id_value_pairs{log_event.get_node_id_value_pairs()};
    clp::ffi::value_int_t timestamp{0};

    if (m_timestamp_node_id.has_value()) {
        auto const& timestamp_pair{id_value_pairs.at(m_timestamp_node_id.value())};
        if (timestamp_pair.has_value()) {
            if (timestamp_pair->is<clp::ffi::value_int_t>()) {
                timestamp = timestamp_pair.value().get_immutable_view<clp::ffi::value_int_t>();
            } else {
                // TODO: Add support for parsing timestamp values of string type.
                SPDLOG_ERROR("Timestamp type is not int");
            }
        }
    }

    LogLevel log_level{LogLevel::NONE};
    if (m_log_level_node_id.has_value()) {
        auto const& log_level_pair{id_value_pairs.at(m_log_level_node_id.value())};
        if (log_level_pair.has_value()) {
            if (log_level_pair->is<std::string>()) {
                auto const& log_level_name
                        = log_level_pair.value().get_immutable_view<std::string>();
                log_level = get_log_level(log_level_name);
            } else if (log_level_pair->is<clp::ffi::value_int_t>()) {
                auto const& value = (log_level_pair.value().get_immutable_view<clp::ffi::value_int_t>());
                if (value <= (clp::enum_to_underlying_type(cValidLogLevelsEndIdx))) {
                    log_level = static_cast<LogLevel>(value);
                }
            } else {
                SPDLOG_ERROR("Log level type is not string");
            }
        }
    }

    auto log_event_with_filter_data{
            LogEventWithFilterData<StructuredLogEvent>(std::move(log_event), log_level, timestamp)
    };

    m_deserialized_log_events->emplace_back(std::move(log_event_with_filter_data));

    return clp::ffi::ir_stream::IRErrorCode::IRErrorCode_Success;
}

auto StructuredIrStreamReader::create(
        std::unique_ptr<ZstdDecompressor>&& zstd_decompressor,
        clp::Array<char> data_array,
        ReaderOptions const& reader_options
) -> StructuredIrStreamReader {
    auto deserialized_log_events{
            std::make_shared<std::vector<LogEventWithFilterData<StructuredLogEvent>>>()
    };
    auto result{StructuredIrDeserializer::create(
            *zstd_decompressor,
            IrUnitHandler{
                    deserialized_log_events,
                    reader_options[cReaderOptionsLogLevelKey.data()].as<std::string>(),
                    reader_options[cReaderOptionsTimestampKey.data()].as<std::string>()
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
    if (false == m_filtered_log_event_map.has_value()) {
        return FilteredLogEventMapTsType{emscripten::val::null()};
    }

    return FilteredLogEventMapTsType{emscripten::val::array(m_filtered_log_event_map.value())};
}

void StructuredIrStreamReader::filter_log_events(LogLevelFilterTsType const& log_level_filter) {
    if (log_level_filter.isNull()) {
        m_filtered_log_event_map.reset();
        return;
    }

    m_filtered_log_event_map.emplace();
    auto filter_levels{emscripten::vecFromJSArray<std::underlying_type_t<LogLevel>>(log_level_filter
    )};
    for (size_t log_event_idx = 0; log_event_idx < m_deserialized_log_events->size();
         ++log_event_idx)
    {
        auto const& log_event = m_deserialized_log_events->at(log_event_idx);
        if (std::ranges::find(
                    filter_levels,
                    clp::enum_to_underlying_type(log_event.get_log_level())
            )
            != filter_levels.end())
        {
            m_filtered_log_event_map->emplace_back(log_event_idx);
        }
    }
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
        if (std::errc::operation_not_permitted == error) {
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
    m_stream_reader_data_context.reset(nullptr);
    return m_deserialized_log_events->size();
}

auto StructuredIrStreamReader::decode_range(size_t begin_idx, size_t end_idx, bool use_filter) const
        -> DecodedResultsTsType {
    if (use_filter) {
        SPDLOG_ERROR(cLogLevelFilteringNotSupportedErrorMsg);
        return DecodedResultsTsType{emscripten::val::null()};
    }

    if (m_deserialized_log_events->size() < end_idx || begin_idx > end_idx) {
        return DecodedResultsTsType{emscripten::val::null()};
    }

    auto const results{emscripten::val::array()};

    for (size_t log_event_idx = begin_idx; log_event_idx < end_idx; ++log_event_idx) {
        auto const& log_event_with_filter_data{m_deserialized_log_events->at(log_event_idx)};
        auto const& structured_log_event = log_event_with_filter_data.get_log_event();

        auto const json_result{structured_log_event.serialize_to_json()};
        std::string json_str{cEmptyJsonStr};
        if (false == json_result.has_value()) {
            auto error_code{json_result.error()};
            SPDLOG_ERROR(
                    "Failed to deserialize log event to JSON: {}:{}",
                    error_code.category().name(),
                    error_code.message()
            );
        } else {
            json_str = json_result.value().dump();
        }

        EM_ASM(
                { Emval.toValue($0).push([UTF8ToString($1), $2, $3, $4]); },
                results.as_handle(),
                json_str.c_str(),
                log_event_with_filter_data.get_timestamp(),
                log_event_with_filter_data.get_log_level(),
                log_event_idx + 1
        );
    }

    return DecodedResultsTsType(results);
}

StructuredIrStreamReader::StructuredIrStreamReader(
        StreamReaderDataContext<StructuredIrDeserializer>&& stream_reader_data_context,
        std::shared_ptr<std::vector<LogEventWithFilterData<StructuredLogEvent>>>
                deserialized_log_events
)
        : m_deserialized_log_events{std::move(deserialized_log_events)},
          m_stream_reader_data_context{
                  std::make_unique<StreamReaderDataContext<StructuredIrDeserializer>>(
                          std::move(stream_reader_data_context)
                  )
          } {}
}  // namespace clp_ffi_js::ir
