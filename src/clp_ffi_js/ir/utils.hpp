#ifndef CLP_FFI_JS_IR_UTILS_HPP
#define CLP_FFI_JS_IR_UTILS_HPP

#include <emscripten/val.h>

#include "clp_ffi_js/ir/StreamReader.hpp"

namespace clp_ffi_js::ir {

template <typename LogEvents>
auto filter_deserialized_events(
        LogLevelFilterTsType log_level_filter,
        FilteredLogEventsMap& filtered_log_event_map,
        LogEvents const& log_events
) -> void;
}  // namespace clp_ffi_js::ir

#endif  // CLP_FFI_JS_IR_UTILS_HPP
