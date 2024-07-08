#ifndef CLP_FFI_JS_TYPES_HPP
#define CLP_FFI_JS_TYPES_HPP

#include <array>

constexpr std::array<std::string_view, 7> cLogLevelNames = {
        "NONE",  // This should not be used.
        "TRACE",
        "DEBUG",
        "INFO",
        "WARN",
        "ERROR",
        "FATAL",
};

#endif  // CLP_FFI_JS_TYPES_HPP
