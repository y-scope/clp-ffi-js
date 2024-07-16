#ifndef CLP_FFI_JS_CONSTANTS_HPP
#define CLP_FFI_JS_CONSTANTS_HPP

#include <array>

namespace clp_ffi_js {
constexpr std::array cLogLevelNames{
        "NONE",  // This should not be used.
        "TRACE",
        "DEBUG",
        "INFO",
        "WARN",
        "ERROR",
        "FATAL",
};
}  // namespace clp_ffi_js

#endif  // CLP_FFI_JS_CONSTANTS_HPP
