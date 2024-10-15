#ifndef CLP_FFI_JS_IR_DECODING_METHODS_HPP
#define CLP_FFI_JS_IR_DECODING_METHODS_HPP

#include <clp/ReaderInterface.hpp>

namespace clp_ffi_js::ir {
/**
 * Rewinds the reader to the beginning and verifies the CLP IR data encoding type.
 * @param reader
 * @throws ClpFfiJsException if there is a failure to decode the encoding type or if the encoding
 * type is unsupported.
 */
auto rewind_reader_and_verify_encoding_type(clp::ReaderInterface& reader) -> void;
}  // namespace clp_ffi_js::ir

#endif  // CLP_FFI_JS_IR_DECODING_METHODS_HPP
