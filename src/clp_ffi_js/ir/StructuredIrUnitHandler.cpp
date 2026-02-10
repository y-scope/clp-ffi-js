#include "StructuredIrUnitHandler.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_utils.hpp>
#include <utility>

#include <clp/ffi/ir_stream/decoding_methods.hpp>
#include <clp/ffi/SchemaTree.hpp>
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
 * @return std::nullopt otherwise.
 */
[[nodiscard]] auto parse_log_level(std::string_view str) -> std::optional<LogLevel>;

/**
 * Parses the log level from the given value.
 * @param value
 * @return The parsed log level forwarded from `parse_log_level`.
 * @return std::nullopt on failures:
 * - The given value's type cannot be decoded as a string.
 * - Forwards `clp::ffi::EncodedTextAst::to_string`'s return values.
 * - Forwards `parse_log_level`'s return values.
 */
[[nodiscard]] auto parse_log_level_from_value(clp::ffi::Value const& value)
        -> std::optional<LogLevel>;

auto parse_log_level(std::string_view str) -> std::optional<LogLevel> {
    // Convert the string to uppercase.
    std::string log_level_name_upper_case{str};
    std::ranges::transform(
            log_level_name_upper_case.begin(),
            log_level_name_upper_case.end(),
            log_level_name_upper_case.begin(),
            [](unsigned char c) { return std::toupper(c); }
    );

    auto const it = std::ranges::find(
            cLogLevelNames.begin() + clp::enum_to_underlying_type(cValidLogLevelsBeginIdx),
            cLogLevelNames.end(),
            log_level_name_upper_case
    );
    if (it == cLogLevelNames.end()) {
        return std::nullopt;
    }

    return static_cast<LogLevel>(std::distance(cLogLevelNames.begin(), it));
}

auto parse_log_level_from_value(clp::ffi::Value const& value) -> std::optional<LogLevel> {
    if (value.is<std::string>()) {
        return parse_log_level(value.get_immutable_view<std::string>());
    }

    if (value.is<clp::ffi::FourByteEncodedTextAst>()) {
        auto const result
                = value.get_immutable_view<clp::ffi::FourByteEncodedTextAst>().to_string();
        if (result.has_error()) {
            auto const& error{result.error()};
            SPDLOG_ERROR(
                    "Failed to decode `clp::ffi::FourByteEncodedTextAst`: {} - {}",
                    error.category().name(),
                    error.message()
            );
            return std::nullopt;
        }
        return parse_log_level(result.value());
    }

    if (value.is<clp::ffi::EightByteEncodedTextAst>()) {
        auto const result
                = value.get_immutable_view<clp::ffi::EightByteEncodedTextAst>().to_string();
        if (result.has_error()) {
            auto const& error{result.error()};
            SPDLOG_ERROR(
                    "Failed to decode `clp::ffi::EightByteEncodedTextAst`: {} - {}",
                    error.category().name(),
                    error.message()
            );
            return std::nullopt;
        }
        return parse_log_level(result.value());
    }

    SPDLOG_ERROR("Protocol Error: The log level value must be a valid string-convertible type.");
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

    auto const optional_node_id{schema_tree.try_get_node_id(leaf_locator)};
    if (false == optional_node_id.has_value()) {
        return false;
    }
    auto node_id{optional_node_id.value()};
    for (auto const& key : m_leaf_to_root_path) {
        auto const& node{schema_tree.get_node(node_id)};
        if (node.is_root()) {
            // Reaching root before matching all the keys in the expected path.
            return false;
        }
        if (node.get_key_name() != key) {
            return false;
        }
        node_id = node.get_parent_id_unsafe();
    }

    if (false == schema_tree.get_node(node_id).is_root()) {
        // The root is not reached yet.
        // The expected leaf-to-root path only matches the bottom branch from the leaf.
        return false;
    }

    return true;
}

auto StructuredIrUnitHandler::handle_log_event(
        StructuredLogEvent&& log_event,
        [[maybe_unused]] size_t log_event_idx
) -> clp::ffi::ir_stream::IRErrorCode {
    auto const timestamp = get_timestamp(log_event);
    auto const log_level = get_log_level(log_event);
    auto const utc_offset = get_utc_offset(log_event);

    m_deserialized_log_events->emplace_back(std::move(log_event), log_level, timestamp, utc_offset);

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

    if (false == m_optional_log_level_node_id.has_value()
        && m_optional_log_level_full_branch.has_value()
        && is_auto_generated == m_optional_log_level_full_branch->is_auto_generated())
    {
        if (m_optional_log_level_full_branch->match(*schema_tree, schema_tree_node_locator)) {
            m_optional_log_level_node_id.emplace(inserted_node_id);
        }
    }

    if (false == m_optional_timestamp_node_id.has_value()
        && m_optional_timestamp_full_branch.has_value()
        && is_auto_generated == m_optional_timestamp_full_branch->is_auto_generated())
    {
        if (m_optional_timestamp_full_branch->match(*schema_tree, schema_tree_node_locator)) {
            m_optional_timestamp_node_id.emplace(inserted_node_id);
        }
    }

    if (false == m_optional_utc_offset_node_id.has_value()
        && m_optional_utc_offset_full_branch.has_value()
        && is_auto_generated == m_optional_utc_offset_full_branch->is_auto_generated())
    {
        if (m_optional_utc_offset_full_branch->match(*schema_tree, schema_tree_node_locator)) {
            m_optional_utc_offset_node_id.emplace(inserted_node_id);
        }
    }

    return clp::ffi::ir_stream::IRErrorCode::IRErrorCode_Success;
}

auto StructuredIrUnitHandler::handle_end_of_stream() const -> clp::ffi::ir_stream::IRErrorCode {
    if (m_optional_log_level_full_branch.has_value()
        && false == m_optional_log_level_node_id.has_value())
    {
        SPDLOG_WARN("Log-level filter option is given, but the key is not found in the IR stream.");
    }
    if (m_optional_timestamp_full_branch.has_value()
        && false == m_optional_timestamp_node_id.has_value())
    {
        SPDLOG_WARN("Timestamp filter option is given, but the key is not found in the IR stream.");
    }
    if (m_optional_utc_offset_full_branch.has_value()
        && false == m_optional_utc_offset_node_id.has_value())
    {
        SPDLOG_WARN(
                "Utc offset filter option is given, but the key is not found in the IR stream."
        );
    }
    return clp::ffi::ir_stream::IRErrorCode::IRErrorCode_Success;
}

auto StructuredIrUnitHandler::get_log_level(StructuredLogEvent const& log_event) const -> LogLevel {
    constexpr LogLevel cDefaultLogLevel{LogLevel::NONE};

    if (false == m_optional_log_level_full_branch.has_value()) {
        return cDefaultLogLevel;
    }

    if (false == m_optional_log_level_node_id.has_value()) {
        return cDefaultLogLevel;
    }

    auto const log_level_node_id = m_optional_log_level_node_id.value();
    auto const& node_id_value_pairs = m_optional_log_level_full_branch->is_auto_generated()
                                              ? log_event.get_auto_gen_node_id_value_pairs()
                                              : log_event.get_user_gen_node_id_value_pairs();
    if (false == node_id_value_pairs.contains(log_level_node_id)) {
        return cDefaultLogLevel;
    }

    auto const& optional_log_level_value = node_id_value_pairs.at(log_level_node_id);
    if (false == optional_log_level_value.has_value()) {
        SPDLOG_ERROR(
                "Protocol error: The log level cannot be an empty value. Log event index: {}",
                m_deserialized_log_events->size()
        );
        return cDefaultLogLevel;
    }

    auto const optional_log_level = parse_log_level_from_value(optional_log_level_value.value());
    if (false == optional_log_level.has_value()) {
        SPDLOG_DEBUG(
                "Failed to parse log level for log event index {}",
                m_deserialized_log_events->size()
        );
        return cDefaultLogLevel;
    }

    return optional_log_level.value();
}

auto StructuredIrUnitHandler::get_timestamp(StructuredLogEvent const& log_event) const
        -> clp::ir::epoch_time_ms_t {
    constexpr clp::ir::epoch_time_ms_t cDefaultTimestamp{0};

    if (false == m_optional_timestamp_full_branch.has_value()) {
        return cDefaultTimestamp;
    }

    if (false == m_optional_timestamp_node_id.has_value()) {
        return cDefaultTimestamp;
    }

    auto const timestamp_node_id = m_optional_timestamp_node_id.value();
    auto const& node_id_value_pairs = m_optional_timestamp_full_branch->is_auto_generated()
                                              ? log_event.get_auto_gen_node_id_value_pairs()
                                              : log_event.get_user_gen_node_id_value_pairs();
    if (false == node_id_value_pairs.contains(timestamp_node_id)) {
        return cDefaultTimestamp;
    }

    auto const& optional_ts = node_id_value_pairs.at(timestamp_node_id);
    if (false == optional_ts.has_value()) {
        SPDLOG_ERROR(
                "Protocol error: The timestamp cannot be an empty value. Log event index: {}",
                m_deserialized_log_events->size()
        );
        return cDefaultTimestamp;
    }

    auto const& timestamp{optional_ts.value()};
    if (false == timestamp.is<clp::ffi::value_int_t>()) {
        SPDLOG_ERROR(
                "Protocol error: The timestamp value must be a valid integer. Log event index: {}",
                m_deserialized_log_events->size()
        );
        return cDefaultTimestamp;
    }
    return static_cast<clp::ir::epoch_time_ms_t>(
            timestamp.get_immutable_view<clp::ffi::value_int_t>()
    );
}

auto StructuredIrUnitHandler::get_utc_offset(StructuredLogEvent const& log_event) const
        -> UtcOffset {
    constexpr std::chrono::minutes cDefaultUtcOffset{0};

    if (false == m_optional_utc_offset_full_branch.has_value()) {
        return cDefaultUtcOffset;
    }

    if (false == m_optional_utc_offset_node_id.has_value()) {
        return cDefaultUtcOffset;
    }

    auto const utc_offset_node_id = m_optional_utc_offset_node_id.value();
    auto const& node_id_value_pairs = m_optional_utc_offset_full_branch->is_auto_generated()
                                              ? log_event.get_auto_gen_node_id_value_pairs()
                                              : log_event.get_user_gen_node_id_value_pairs();
    if (false == node_id_value_pairs.contains(utc_offset_node_id)) {
        return cDefaultUtcOffset;
    }

    auto const& optional_utc_offset_value = node_id_value_pairs.at(utc_offset_node_id);
    if (false == optional_utc_offset_value.has_value()) {
        SPDLOG_ERROR(
                "Protocol error: The utc offset cannot be an empty value. Log event index: {}",
                m_deserialized_log_events->size()
        );
        return cDefaultUtcOffset;
    }

    auto const& utc_offset_value{optional_utc_offset_value.value()};
    if (false == utc_offset_value.is<clp::ffi::value_int_t>()) {
        SPDLOG_ERROR(
                "Protocol error: The utc offset must be a valid integer. Log event index: {}",
                m_deserialized_log_events->size()
        );
        return cDefaultUtcOffset;
    }
    // TODO: Support parsing of other time units, e.g., minutes, hours, etc..
    auto const utc_offset_seconds{
            std::chrono::seconds{utc_offset_value.get_immutable_view<clp::ffi::value_int_t>()}
    };
    return std::chrono::duration_cast<UtcOffset>(utc_offset_seconds);
}
}  // namespace clp_ffi_js::ir
