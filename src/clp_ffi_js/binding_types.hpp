#ifndef CLP_FFI_JS_BINDING_TYPES_HPP
#define CLP_FFI_JS_BINDING_TYPES_HPP

#include <emscripten/val.h>

namespace clp_ffi_js {
// JS types used as inputs
EMSCRIPTEN_DECLARE_VAL_TYPE(DataArrayTsType);

// JS types used as outputs
EMSCRIPTEN_DECLARE_VAL_TYPE(StringArrayTsType);
}  // namespace clp_ffi_js

#endif  // CLP_FFI_JS_BINDING_TYPES_HPP
