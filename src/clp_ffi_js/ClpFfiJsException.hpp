#ifndef CLP_FFI_JS_CLPFFIJSEXCEPTION_HPP
#define CLP_FFI_JS_CLPFFIJSEXCEPTION_HPP

#include <string>
#include <utility>

#include <clp/ErrorCode.hpp>
#include <clp/TraceableException.hpp>

namespace clp_ffi_js {
class ClpFfiJsException : public clp::TraceableException {
public:
    // Constructors
    ClpFfiJsException(
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
}  // namespace clp_ffi_js

#endif  // CLP_FFI_JS_CLPFFIJSEXCEPTION_HPP
