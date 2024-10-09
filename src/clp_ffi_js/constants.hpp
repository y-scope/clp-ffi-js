#ifndef CLP_FFI_JS_CONSTANTS_HPP
#define CLP_FFI_JS_CONSTANTS_HPP

#include <array>
#include <cstdint>
#include <string_view>

namespace clp_ffi_js {
/**
* Enum of known log levels.
*/
enum class LogLevel : std::uint8_t {
    NONE = 0,
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL,
};
constexpr LogLevel cValidLogLevelsBeginIdx{LogLevel::TRACE};

/**
* Strings corresponding to `LogLevel`.
*
* NOTE: These must be kept in sync manually.
*/
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
