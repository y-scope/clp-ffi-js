#ifndef CLP_FFI_JS_IR_DECODING_METHODS_HPP
#define CLP_FFI_JS_IR_DECODING_METHODS_HPP

#include <string>

#include <clp/ReaderInterface.hpp>

namespace clp_ffi_js::ir {
/**
 * Gets the version of the IR stream from the specified reader.
 * @param reader
 * @throws ClpFfiJsException if the encoding type couldn't be decoded or the encoding type is
 * unsupported.
 * @throws ClpFfiJsException if the preamble could not be deserialized.
 * @return Stream version.
 */
auto get_version(clp::ReaderInterface& reader) -> std::string;
/**
 * Rewinds the reader to the beginning and validates the CLP IR data encoding type.
 * @param reader
 * @throws ClpFfiJsException if the encoding type couldn't be decoded or the encoding type is
 * unsupported.
 */
auto rewind_reader_and_validate_encoding_type(clp::ReaderInterface& reader) -> void;
}  // namespace clp_ffi_js::ir

#endif  // CLP_FFI_JS_IR_DECODING_METHODS_HPP
