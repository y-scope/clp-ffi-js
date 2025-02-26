#include "StructuredIrStreamReader.hpp"

#include <cstddef>
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#include <clp/Array.hpp>
#include <clp/ErrorCode.hpp>
#include <clp/ffi/ir_stream/Deserializer.hpp>
#include <clp/ffi/SchemaTree.hpp>
#include <clp/ir/types.hpp>
#include <clp/TraceableException.hpp>
#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <json/single_include/nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <clp_ffi_js/ClpFfiJsException.hpp>
#include <clp_ffi_js/ir/LogEventWithFilterData.hpp>
#include <clp_ffi_js/ir/StreamReader.hpp>
#include <clp_ffi_js/ir/StreamReaderDataContext.hpp>
#include <clp_ffi_js/ir/StructuredIrUnitHandler.hpp>

namespace clp_ffi_js::ir {
namespace {
constexpr std::string_view cEmptyJsonStr{"{}"};
constexpr std::string_view cReaderOptionIsAutoGeneratedKey{"isAutoGenerated"};
constexpr std::string_view cReaderOptionPartsKey{"parts"};
constexpr std::string_view cReaderOptionsLogLevelKey{"logLevelKey"};
constexpr std::string_view cReaderOptionsTimestampKey{"timestampKey"};
constexpr std::string_view cMergedKvPairsAutoGeneratedKey{"auto-generated"};
constexpr std::string_view cMergedKvPairsUserGeneratedKey{"user-generated"};

/**
 * @see nlohmann::basic_json::dump
 * Serializes a JSON value into a string with invalid UTF-8 sequences replaced rather than
 * throwing an exception.
 * @param json
 * @return Serialized JSON.
 */
[[nodiscard]] auto dump_json_with_replace(nlohmann::json const& json) -> std::string;

/**
 * @param option The JavaScript object representing the reader option.
 * @param leaf_node_type The type of the leaf node in the constructed schema tree branch.
 * @return A schema tree full branch constructed based on the provided reader option.
 * @return std::nullopt if `option` is `null`.
 */
[[nodiscard]] auto get_schema_tree_full_path_from_js_reader_option(
        emscripten::val const& option,
        clp::ffi::SchemaTree::Node::Type leaf_node_type
) -> std::optional<StructuredIrUnitHandler::SchemaTreeFullBranch>;

auto dump_json_with_replace(nlohmann::json const& json) -> std::string {
    return json.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
}

auto get_schema_tree_full_path_from_reader_option(
        emscripten::val const& option,
        clp::ffi::SchemaTree::Node::Type leaf_node_type
) -> std::optional<StructuredIrUnitHandler::SchemaTreeFullBranch> {
    if (option.isNull()) {
        return std::nullopt;
    }
    return StructuredIrUnitHandler::SchemaTreeFullBranch{
            option[cReaderOptionIsAutoGeneratedKey.data()].as<bool>(),
            emscripten::vecFromJSArray<std::string>(option[cReaderOptionPartsKey.data()]),
            leaf_node_type
    };
}

EMSCRIPTEN_BINDINGS(ClpStructuredIrStreamReader) {
    emscripten::constant(
            "MERGED_KV_PAIRS_AUTO_GENERATED_KEY",
            std::string{cMergedKvPairsAutoGeneratedKey}
    );
    emscripten::constant(
            "MERGED_KV_PAIRS_USER_GENERATED_KEY",
            std::string{cMergedKvPairsUserGeneratedKey}
    );
}
}  // namespace

auto StructuredIrStreamReader::create(
        std::unique_ptr<ZstdDecompressor>&& zstd_decompressor,
        clp::Array<char> data_array,
        ReaderOptions const& reader_options
) -> StructuredIrStreamReader {
    auto deserialized_log_events{std::make_shared<StructuredLogEvents>()};
    auto result{StructuredIrDeserializer::create(
            *zstd_decompressor,
            StructuredIrUnitHandler{
                    deserialized_log_events,
                    get_schema_tree_full_path_from_reader_option(
                            reader_options[cReaderOptionsLogLevelKey.data()],
                            clp::ffi::SchemaTree::Node::Type::Str
                    ),
                    get_schema_tree_full_path_from_reader_option(
                            reader_options[cReaderOptionsTimestampKey.data()],
                            clp::ffi::SchemaTree::Node::Type::Int
                    )
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
    generic_filter_log_events(
            m_filtered_log_event_map,
            log_level_filter,
            *m_deserialized_log_events
    );
}

auto StructuredIrStreamReader::deserialize_stream() -> size_t {
    if (nullptr == m_stream_reader_data_context) {
        return m_deserialized_log_events->size();
    }

    constexpr size_t cDefaultNumReservedLogEvents{500'000};
    m_deserialized_log_events->reserve(cDefaultNumReservedLogEvents);
    auto& reader{m_stream_reader_data_context->get_reader()};
    auto& deserializer = m_stream_reader_data_context->get_deserializer();

    while (false == deserializer.is_stream_completed()) {
        auto result{deserializer.deserialize_next_ir_unit(reader)};
        if (false == result.has_error()) {
            continue;
        }
        auto const error{result.error()};
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
    auto log_event_to_string = [](StructuredLogEvent const& log_event) -> std::string {
        auto json_pair_result{log_event.serialize_to_json()};
        if (json_pair_result.has_error()) {
            auto const error_code{json_pair_result.error()};
            SPDLOG_ERROR(
                    "Failed to deserialize log event to JSON: {}:{}",
                    error_code.category().name(),
                    error_code.message()
            );
            return std::string{cEmptyJsonStr};
        }

        auto& [auto_generated, user_generated] = json_pair_result.value();
        nlohmann::json const merged_kv_pairs
                = {{std::string{cMergedKvPairsAutoGeneratedKey}, std::move(auto_generated)},
                   {std::string{cMergedKvPairsUserGeneratedKey}, std::move(user_generated)}};
        return dump_json_with_replace(merged_kv_pairs);
    };

    return generic_decode_range(
            begin_idx,
            end_idx,
            m_filtered_log_event_map,
            *m_deserialized_log_events,
            log_event_to_string,
            use_filter
    );
}

auto StructuredIrStreamReader::find_nearest_log_event_by_timestamp(
        clp::ir::epoch_time_ms_t const target_ts
) -> NullableLogEventIdx {
    return generic_find_nearest_log_event_by_timestamp(*m_deserialized_log_events, target_ts);
}

StructuredIrStreamReader::StructuredIrStreamReader(
        StreamReaderDataContext<StructuredIrDeserializer>&& stream_reader_data_context,
        std::shared_ptr<StructuredLogEvents> deserialized_log_events
)
        : m_deserialized_log_events{std::move(deserialized_log_events)},
          m_stream_reader_data_context{
                  std::make_unique<StreamReaderDataContext<StructuredIrDeserializer>>(
                          std::move(stream_reader_data_context)
                  )
          } {}
}  // namespace clp_ffi_js::ir
