#ifndef CLP_FFI_JS_CONSTANTS_HPP
#define CLP_FFI_JS_CONSTANTS_HPP

#include <array>

namespace clp_ffi_js {
using namespace std::literals::string_view_literals;

constexpr std::array cLogLevelNames{
        "NONE"sv,  // This should not be used.
        "TRACE"sv,
        "DEBUG"sv,
        "INFO"sv,
        "WARN"sv,
        "ERROR"sv,
        "FATAL"sv,
};
}  // namespace clp_ffi_js

#endif  // CLP_FFI_JS_CONSTANTS_HPP
