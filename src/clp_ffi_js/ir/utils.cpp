#include "utils.hpp"

#include <algorithm>
#include <cstddef>
#include <type_traits>
#include <vector>

#include <clp/type_utils.hpp>
#include <emscripten/val.h>

#include <clp_ffi_js/constants.hpp>
#include <clp_ffi_js/ir/StreamReader.hpp>

namespace clp_ffi_js::ir {
template <typename LogEvents>
auto filter_deserialized_events(
        LogLevelFilterTsType const& log_level_filter,
        FilteredLogEventsMap& filtered_log_event_map,
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
