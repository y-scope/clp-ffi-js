#ifndef CLP_FFI_JS_TYPES_HPP
#define CLP_FFI_JS_TYPES_HPP

#include <array>
#include <string>
#include <utility>

using namespace std::string_view_literals;

constexpr std::array cLogLevelNames = {
        "NONE"sv,  // This should not be used.
        "TRACE"sv,
        "DEBUG"sv,
        "INFO"sv,
        "WARN"sv,
        "ERROR"sv,
        "FATAL"sv,
};

class ClpJsException : public clp::TraceableException {
public:
    // Constructors
    ClpJsException(
            clp::ErrorCode error_code,
            char const* const filename,
            int line_number,
            std::string message
    )
            : TraceableException{error_code, filename, line_number},
              m_message{std::move(message)} {}

    // Methods
    [[nodiscard]] auto what() const noexcept -> char const* override { return m_message.c_str(); }

private:
    std::string m_message;
};

#endif  // CLP_FFI_JS_TYPES_HPP
