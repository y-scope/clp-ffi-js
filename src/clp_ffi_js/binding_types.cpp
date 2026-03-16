#include <emscripten/bind.h>

#include <clp_ffi_js/binding_types.hpp>

namespace clp_ffi_js {
EMSCRIPTEN_BINDINGS(ClpFfiJsBindingTypes) {
    // JS types used as inputs
    emscripten::register_type<DataArrayTsType>("Uint8Array");
}
}  // namespace clp_ffi_js
