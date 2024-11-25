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
#include <clp/ffi/Value.hpp>
#include <emscripten/val.h>
#include <spdlog/spdlog.h>

#include <clp_ffi_js/constants.hpp>
#include <clp_ffi_js/ir/LogEventWithFilterData.hpp>

namespace clp_ffi_js::ir {

namespace {
/**
 * Parses a string to determine the corresponding `LogLevel` enum value.
 * @param str
 * @return `LogLevel` enum corresponding to a string in `cLogLevelNames`. If input string
 * is not in `cLogLevelNames`, returns `LogLevel::NONE`.
 *
 */
auto parse_log_level(std::string_view str) -> LogLevel;

/**
 * Parses a string to determine the corresponding `LogLevel` enumeration value.
 *
 * @param str It should match one of the valid log level names in `cLogLevelNames`.
 * @return The parsed value if the input is valid.
 *         If invalid, returns `LogLevel::NONE`.
 */

auto parse_log_level(std::string_view str) -> LogLevel {
    LogLevel log_level{LogLevel::NONE};

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
}  // namespace

auto StructuredIrUnitHandler::handle_schema_tree_node_insertion(
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

auto StructuredIrUnitHandler::handle_log_event(StructuredLogEvent&& log_event
) -> clp::ffi::ir_stream::IRErrorCode {
    auto const& id_value_pairs{log_event.get_node_id_value_pairs()};
    auto const timestamp = get_timestamp(id_value_pairs);
    auto const log_level = get_log_level(id_value_pairs);

    m_deserialized_log_events->emplace_back(std::move(log_event), log_level, timestamp);

    return clp::ffi::ir_stream::IRErrorCode::IRErrorCode_Success;
}

auto StructuredIrUnitHandler::get_log_level(
        StructuredLogEvent::NodeIdValuePairs const& id_value_pairs
) const -> LogLevel {
    LogLevel log_level{LogLevel::NONE};

    if (false == m_log_level_node_id.has_value()) {
        return log_level;
    }

    auto const& log_level_pair{id_value_pairs.at(m_log_level_node_id.value())};

    if (false == log_level_pair.has_value()) {
        return log_level;
    }

    if (log_level_pair->is<std::string>()) {
        auto const& log_level_node_value = log_level_pair.value().get_immutable_view<std::string>();
        log_level = parse_log_level(log_level_node_value);
    } else if (log_level_pair->is<clp::ffi::value_int_t>()) {
        auto const& log_level_node_value
                = (log_level_pair.value().get_immutable_view<clp::ffi::value_int_t>());
        if (log_level_node_value >= clp::enum_to_underlying_type(cValidLogLevelsBeginIdx)
            && log_level_node_value < clp::enum_to_underlying_type(cValidLogLevelsEndIdx))
        {
            log_level = static_cast<LogLevel>(log_level_node_value);
        }
    } else {
        auto log_event_idx = m_deserialized_log_events->size();
        SPDLOG_ERROR("Log level type is not int or string for log event index {}", log_event_idx);
    }

    return log_level;
}

auto StructuredIrUnitHandler::get_timestamp(
        StructuredLogEvent::NodeIdValuePairs const& id_value_pairs
) const -> clp::ffi::value_int_t {
    clp::ffi::value_int_t timestamp{0};

    if (false == m_timestamp_node_id.has_value()) {
        return timestamp;
    }

    auto const& timestamp_pair{id_value_pairs.at(m_timestamp_node_id.value())};

    if (false == timestamp_pair.has_value()) {
        return timestamp;
    }

    if (timestamp_pair->is<clp::ffi::value_int_t>()) {
        timestamp = timestamp_pair.value().get_immutable_view<clp::ffi::value_int_t>();
    } else {
        // TODO: Add support for parsing timestamp values of string type.
        auto log_event_idx = m_deserialized_log_events->size();
        SPDLOG_ERROR("Timestamp type is not int for log event index {}", log_event_idx);
    }

    return timestamp;
}
}  // namespace clp_ffi_js::ir
