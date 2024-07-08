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

class DecodingException : public clp::TraceableException {
public:
    // Constructors
    DecodingException(
            clp::ErrorCode error_code,
            char const* const filename,
            int line_number,
            std::string message
    )
            : TraceableException(error_code, filename, line_number),
              m_message(std::move(message)) {}

    // Methods
    [[nodiscard]] char const* what() const noexcept override { return m_message.c_str(); }

private:
    std::string m_message;
};

#endif  // CLP_FFI_JS_TYPES_HPP
