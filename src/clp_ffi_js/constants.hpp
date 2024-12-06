#ifndef CLP_FFI_JS_CONSTANTS_HPP
#define CLP_FFI_JS_CONSTANTS_HPP

#include <array>
#include <cstdint>
#include <string_view>
#include <type_utils.hpp>

namespace clp_ffi_js {
/**
 * Enum of known log levels.
 */
enum class LogLevel : std::uint8_t {
    NONE = 0,  // This isn't a valid log level.
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL,
    LENGTH,  // This isn't a valid log level.
};
constexpr LogLevel cValidLogLevelsBeginIdx{LogLevel::TRACE};

/**
 * Strings corresponding to `LogLevel`.
 *
 * NOTE: These must be kept in sync manually.
 */
constexpr std::array<std::string_view, clp::enum_to_underlying_type(LogLevel::LENGTH)>
        cLogLevelNames{
                "NONE",  // This isn't a valid log level.
                "TRACE",
                "DEBUG",
                "INFO",
                "WARN",
                "ERROR",
                "FATAL",
        };
}  // namespace clp_ffi_js

#endif  // CLP_FFI_JS_CONSTANTS_HPP
