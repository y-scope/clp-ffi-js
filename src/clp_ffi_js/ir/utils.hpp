#ifndef CLP_FFI_JS_IR_UTILS_HPP
#define CLP_FFI_JS_IR_UTILS_HPP

#include <algorithm>
#include <cstddef>
#include <type_traits>
#include <vector>

#include <clp/type_utils.hpp>
#include <emscripten/val.h>

#include <clp_ffi_js/constants.hpp>
#include <clp_ffi_js/ir/StreamReader.hpp>

namespace clp_ffi_js::ir {

/**
 * Generates a filtered collection from all log events.
 *
 * @param log_level_filter Array of selected log levels.
 * @param log_events
 * @param[out] filtered_log_event_map A reference to `FilteredLogEventsMap` that stores filtered
 * result.
 */
template <typename LogEvents>
auto filter_deserialized_events(
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
}  // namespace clp_ffi_js::ir

#endif  // CLP_FFI_JS_IR_UTILS_HPP
