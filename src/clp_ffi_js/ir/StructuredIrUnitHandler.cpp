#include "StructuredIrUnitHandler.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <iterator>
#include <memory>
#include <string>
#include <string_view>
#include <type_utils.hpp>
#include <utility>

#include <clp/ffi/ir_stream/decoding_methods.hpp>
#include <clp/ffi/KeyValuePairLogEvent.hpp>
#include <clp/ffi/SchemaTree.hpp>
#include <clp/ir/EncodedTextAst.hpp>
#include <clp/ffi/Value.hpp>
#include <clp/ir/types.hpp>
#include <clp/time_types.hpp>
#include <emscripten/val.h>
#include <spdlog/spdlog.h>

#include <clp_ffi_js/constants.hpp>
#include <clp_ffi_js/ir/LogEventWithFilterData.hpp>

namespace clp_ffi_js::ir {
namespace {
/**
 * Parses a string to determine the corresponding `LogLevel` enum value.
 * @param str
 * @return `LogLevel` enum corresponding to `str` if `str` matches a string in `cLogLevelNames`.
 * @return `LogLevel::NONE` otherwise.
 */
auto parse_log_level(std::string_view str) -> LogLevel;

/**
 * Parses the log level from the given value.
 * @param value
 * @return The parsed log level forwarded from `parse_log_level`.
 * @return std::nullopt on failures:
 * - The given value's type cannot be decoded as a string.
 * - Forwards `clp::ir::EncodedTextAst::decode_and_unparse`'s return values.
 */
[[nodiscard]] auto parse_log_level_from_value(clp::ffi::Value const& value) -> std::optional<LogLevel>;

auto parse_log_level(std::string_view str) -> LogLevel {
    // Convert the string to uppercase.
    std::string log_level_name_upper_case{str};
    std::ranges::transform(
            log_level_name_upper_case.begin(),
            log_level_name_upper_case.end(),
            log_level_name_upper_case.begin(),
            [](unsigned char c) { return std::toupper(c); }
    );

    auto const* it = std::ranges::find(
            cLogLevelNames.begin() + clp::enum_to_underlying_type(cValidLogLevelsBeginIdx),
            cLogLevelNames.end(),
            log_level_name_upper_case
    );
    if (it == cLogLevelNames.end()) {
        return LogLevel::NONE;
    }

    return static_cast<LogLevel>(std::distance(cLogLevelNames.begin(), it));
}

auto parse_log_level_from_value(clp::ffi::Value const& value) -> std::optional<LogLevel> {
    if (value.is<std::string>()) {
        return parse_log_level(value.get_immutable_view<std::string>());
    } else if (value.is<clp::ir::FourByteEncodedTextAst>()) {
        auto const optional_log_level = value.get_immutable_view<clp::ir::FourByteEncodedTextAst>().decode_and_unparse();
        if (false == optional_log_level.has_value()) {
            return std::nullopt;
        }
    } else if (value.is<clp::ir::EightByteEncodedTextAst>()) {
        if (false == optional_log_level.has_value()) {
            return std::nullopt;
        }
        return value.get_immutable_view<clp::ir::EightByteEncodedTextAst>().decode_and_unparse();
    }
    // TODO: We may need to log here
    return std::nullopt;
}
}  // namespace

auto StructuredIrUnitHandler::SchemaTreeFullBranch::match(
        clp::ffi::SchemaTree const& schema_tree,
        clp::ffi::SchemaTree::NodeLocator const& leaf_locator
) const -> bool {
    if (leaf_locator.get_type() != m_leaf_type) {
        return false;
    }

    auto const optional_id{schema_tree.try_get_node_id(leaf_locator)};
    if (false == optional_id.has_value()) {
        return false;
    }
    auto node_id{optional_id.value()};
    size_t matched_depth{0};
    for (auto const& key : m_leaf_to_root_path) {
        auto const& node{schema_tree.get_node(node_id)};
        if (node.get_key_name() != key) {
            return false;
        }
        ++matched_depth;
        auto const optional_parent_id{node.get_parent_id()};
        if (false == optional_parent_id.has_value()) {
            break;
        }
        node_id = optional_parent_id.value();
    }

    if (matched_depth != m_leaf_to_root_path.size()) {
        return false;
    }

    return clp::ffi::SchemaTree::cRootId == node_id;
}

auto StructuredIrUnitHandler::handle_log_event(StructuredLogEvent&& log_event
) -> clp::ffi::ir_stream::IRErrorCode {
    auto const timestamp = get_timestamp(
            m_timestamp_full_branch.is_auto_generated()
                    ? log_event.get_auto_gen_node_id_value_pairs()
                    : log_event.get_user_gen_node_id_value_pairs()
    );
    auto const log_level = get_log_level(
            m_log_level_full_branch.is_auto_generated()
                    ? log_event.get_auto_gen_node_id_value_pairs()
                    : log_event.get_user_gen_node_id_value_pairs()
    );

    m_deserialized_log_events->emplace_back(std::move(log_event), log_level, timestamp);

    return clp::ffi::ir_stream::IRErrorCode::IRErrorCode_Success;
}

auto StructuredIrUnitHandler::handle_utc_offset_change(
        [[maybe_unused]] clp::UtcOffset utc_offset_old,
        [[maybe_unused]] clp::UtcOffset utc_offset_new
) -> clp::ffi::ir_stream::IRErrorCode {
    SPDLOG_WARN("UTC offset change packets aren't handled currently.");
    return clp::ffi::ir_stream::IRErrorCode::IRErrorCode_Success;
}

auto StructuredIrUnitHandler::handle_schema_tree_node_insertion(
        bool is_auto_generated,
        clp::ffi::SchemaTree::NodeLocator schema_tree_node_locator,
        std::shared_ptr<clp::ffi::SchemaTree const> const& schema_tree
) -> clp::ffi::ir_stream::IRErrorCode {
    auto const optional_inserted_node_id{schema_tree->try_get_node_id(schema_tree_node_locator)};
    if (false == optional_inserted_node_id.has_value()) {
        return clp::ffi::ir_stream::IRErrorCode_Corrupted_IR;
    }
    auto const inserted_node_id{optional_inserted_node_id.value()};

    if (false == m_log_level_node_id.has_value()
        && is_auto_generated == m_log_level_full_branch.is_auto_generated())
    {
        if (m_log_level_full_branch.match(*schema_tree, schema_tree_node_locator)) {
            m_log_level_node_id.emplace(inserted_node_id);
        }
    }

    if (false == m_timestamp_node_id.has_value()
        && is_auto_generated == m_timestamp_full_branch.is_auto_generated())
    {
        if (m_timestamp_full_branch.match(*schema_tree, schema_tree_node_locator)) {
            m_timestamp_node_id.emplace(inserted_node_id);
        }
    }

    return clp::ffi::ir_stream::IRErrorCode::IRErrorCode_Success;
}

auto StructuredIrUnitHandler::handle_end_of_stream() -> clp::ffi::ir_stream::IRErrorCode {
    return clp::ffi::ir_stream::IRErrorCode::IRErrorCode_Success;
}

auto StructuredIrUnitHandler::get_log_level(
        StructuredLogEvent::NodeIdValuePairs const& id_value_pairs
) const -> LogLevel {
    constexpr LogLevel cDefaultLogLevel{LogLevel::NONE};

    if (false == m_log_level_node_id.has_value()) {
        return cDefaultLogLevel;
    }
    auto const& optional_log_level_value{id_value_pairs.at(m_log_level_node_id.value())};
    if (false == optional_log_level_value.has_value()) {
        return cDefaultLogLevel;
    }

    auto const optional_log_level{decode_as_str(optional_log_level_value.value())};
    if (false == optional_log_level_value.has_value()) {
        auto const log_event_idx = m_deserialized_log_events->size();
        SPDLOG_INFO("Failed to decode the log level as a string for log event index {}", log_event_idx);
        return cDefaultLogLevel;
    }

    auto const log_level_str = log_level_value.get_immutable_view<std::string>();
    log_level = parse_log_level(log_level_str);
    SPDLOG_INFO("Parsed log level: {}", log_level_str);

    if (log_level_value.is<std::string>()) {
    } else if (log_level_value.is<clp::ffi::value_int_t>()) {
        auto const& log_level_int = log_level_value.get_immutable_view<clp::ffi::value_int_t>();
        if (log_level_int >= clp::enum_to_underlying_type(cValidLogLevelsBeginIdx)
            && log_level_int < clp::enum_to_underlying_type(LogLevel::LENGTH))
        {
            log_level = static_cast<LogLevel>(log_level_int);
        }
    } else {
        auto log_event_idx = m_deserialized_log_events->size();
        SPDLOG_ERROR(
                "Authoritative log level's value is not an int or string for log event index {}",
                log_event_idx
        );
        // TODO: This should be no longer possible
    }

    return log_level;
}

auto StructuredIrUnitHandler::get_timestamp(
        StructuredLogEvent::NodeIdValuePairs const& id_value_pairs
) const -> clp::ir::epoch_time_ms_t {
    constexpr clp::ir::epoch_time_ms_t cDefaultTimestamp{0};
    if (false == m_timestamp_node_id.has_value()) {
        return cDefaultTimestamp;
    }
    auto const& optional_timestmap{id_value_pairs.at(m_timestamp_node_id.value())};
    if (false == optional_timestmap.has_value()) {
        return cDefaultTimestamp;
    }
    auto const& timestamp{optional_timestmap.value()};
    if (false == timestamp.is<clp::ffi::value_int_t>()) {
        // TODO: We need to log this branch as an internal error, or we could just let
        // `get_immutable_view` throw
        return cDefaultTimestamp;
    }
    return static_cast<clp::ir::epoch_time_ms_t>(
            timestamp.get_immutable_view<clp::ffi::value_int_t>()
    );
}
}  // namespace clp_ffi_js::ir
