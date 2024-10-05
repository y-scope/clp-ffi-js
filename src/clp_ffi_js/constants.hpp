#ifndef CLP_FFI_JS_CONSTANTS_HPP
#define CLP_FFI_JS_CONSTANTS_HPP

#include <array>
#include <string_view>

namespace clp_ffi_js {
enum class LogLevel {
    NONE,
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL,
};
constexpr LogLevel cValidLogLevelsBeginIdx{LogLevel::TRACE};
constexpr std::array<std::string_view, 7> cLogLevelNames{
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
