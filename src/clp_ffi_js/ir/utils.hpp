#ifndef CLP_FFI_JS_IR_UTILS_HPP
#define CLP_FFI_JS_IR_UTILS_HPP

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include <clp/ErrorCode.hpp>
#include <clp/TimestampPattern.hpp>
#include <clp/TraceableException.hpp>
#include <clp/type_utils.hpp>
#include <emscripten/em_asm.h>
#include <emscripten/val.h>

#include <clp_ffi_js/ClpFfiJsException.hpp>
#include <clp_ffi_js/constants.hpp>
#include <clp_ffi_js/ir/StreamReader.hpp>
#include <clp_ffi_js/ir/StructuredIrStreamReader.hpp>
#include <clp_ffi_js/ir/UnstructuredIrStreamReader.hpp>

namespace clp_ffi_js::ir {
constexpr std::string_view cEmptyJsonStr{"{}"};

/**
 * A templated function that implements `StreamReader::filter_log_events` for both
 * `UnstructuredIrStreamReader` and `StructuredIrStreamReader`. The additional arguments,
 * which are not part of `filter_log_events`, are private members of the derived
 * `StreamReader` classes.
 *
 * @param log_level_filter
 * @param log_events
 * @param[out] filtered_log_event_map A reference to `FilteredLogEventsMap` that stores filtered
 * result.
 */
template <typename LogEvents>
auto generic_filter_log_events(
        FilteredLogEventsMap& filtered_log_event_map,
        LogLevelFilterTsType const& log_level_filter,
        LogEvents const& log_events
) -> void {
    if (log_level_filter.isNull()) {
        filtered_log_event_map.reset();
        return;
    }

    filtered_log_event_map.emplace();
    auto filter_levels
            = emscripten::vecFromJSArray<std::underlying_type_t<LogLevel>>(log_level_filter);

    for (size_t log_event_idx = 0; log_event_idx < log_events.size(); ++log_event_idx) {
        auto const& log_event = log_events[log_event_idx];
        if (std::ranges::find(
                    filter_levels,
                    clp::enum_to_underlying_type(log_event.get_log_level())
            )
            != filter_levels.end())
        {
            filtered_log_event_map->emplace_back(log_event_idx);
        }
    }
}

/**
 * A templated function that implements `StreamReader::decode_range` for both
 * `UnstructuredIrStreamReader` and `StructuredIrStreamReader`. The additional
 * arguments, which are not part of `decode_range`, are private members of
 * the derived `StreamReader` classes.
 *
 * @param begin_idx
 * @param end_idx
 * @param filtered_log_event_map
 * @param log_events
 * @param use_filter
 * @param ts_pattern A pattern for formatting unstructured log event timestamps as date strings.
 * The pattern is unused for structured log events, as they are not formatted in `clp-ffi-js`.
 * @return
 */
template <typename LogEvents>
auto generic_decode_range(
        size_t begin_idx,
        size_t end_idx,
        FilteredLogEventsMap const& filtered_log_event_map,
        LogEvents const& log_events,
        bool use_filter,
        clp::TimestampPattern const& ts_pattern
) -> DecodedResultsTsType {
    if (use_filter && false == filtered_log_event_map.has_value()) {
        return DecodedResultsTsType{emscripten::val::null()};
    }

    size_t length{0};
    if (use_filter) {
        length = filtered_log_event_map->size();
    } else {
        length = log_events.size();
    }
    if (length < end_idx || begin_idx > end_idx) {
        return DecodedResultsTsType{emscripten::val::null()};
    }

    auto const results{emscripten::val::array()};
    std::string generic_event_string;

    for (size_t i = begin_idx; i < end_idx; ++i) {
        size_t log_event_idx{0};
        if (use_filter) {
            log_event_idx = filtered_log_event_map->at(i);
        } else {
            log_event_idx = i;
        }

        auto const& log_event_with_filter_data{log_events.at(log_event_idx)};
        auto const& log_event = log_event_with_filter_data.get_log_event();
        auto const& timestamp = log_event_with_filter_data.get_timestamp();
        auto const& log_level = log_event_with_filter_data.get_log_level();

        if constexpr (std::is_same_v<LogEvents, UnstructuredLogEvents>) {
            constexpr size_t cDefaultReservedLength{512};
            generic_event_string.reserve(cDefaultReservedLength);

            auto const parsed{log_event.get_message().decode_and_unparse()};
            if (false == parsed.has_value()) {
                throw ClpFfiJsException{
                        clp::ErrorCode::ErrorCode_Failure,
                        __FILENAME__,
                        __LINE__,
                        "Failed to decode message"
                };
            }
            generic_event_string = parsed.value();
            ts_pattern.insert_formatted_timestamp(timestamp, generic_event_string);
        } else if constexpr (std::is_same_v<LogEvents, StructuredLogEvents>) {
            auto const json_result{log_event.serialize_to_json()};
            if (false == json_result.has_value()) {
                auto error_code{json_result.error()};
                SPDLOG_ERROR(
                        "Failed to deserialize log event to JSON: {}:{}",
                        error_code.category().name(),
                        error_code.message()
                );
                generic_event_string = std::string(cEmptyJsonStr);
            } else {
                generic_event_string = json_result.value().dump();
            }
        }

        EM_ASM(
                { Emval.toValue($0).push([UTF8ToString($1), $2, $3, $4]); },
                results.as_handle(),
                generic_event_string.c_str(),
                timestamp,
                log_level,
                log_event_idx + 1
        );
    }

    return DecodedResultsTsType(results);
}
}  // namespace clp_ffi_js::ir

#endif  // CLP_FFI_JS_IR_UTILS_HPP
