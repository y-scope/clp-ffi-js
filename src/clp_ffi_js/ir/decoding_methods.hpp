#ifndef CLP_FFI_JS_IR_DECODING_METHODS_HPP
#define CLP_FFI_JS_IR_DECODING_METHODS_HPP

#include <string>

#include <clp/ReaderInterface.hpp>

namespace clp_ffi_js::ir {
auto get_version(clp::ReaderInterface& reader) -> std::string;
auto rewind_reader_and_verify_encoding_type(clp::ReaderInterface& reader) -> void;
}  // namespace clp_ffi_js::ir

#endif  // CLP_FFI_JS_IR_DECODING_METHODS_HPP
