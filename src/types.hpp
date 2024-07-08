#ifndef CLP_FFI_JS_TYPES_HPP
#define CLP_FFI_JS_TYPES_HPP

#include <array>

constexpr std::array<char const*, 7> cLogLevelNames = {
        nullptr,
        "TRACE",
        "DEBUG",
        "INFO",
        "WARN",
        "ERROR",
        "FATAL",
};

#endif  // CLP_FFI_JS_TYPES_HPP
