#ifndef CLP_FFI_JS_IR_DECODING_METHODS_HPP
#define CLP_FFI_JS_IR_DECODING_METHODS_HPP

#include <nlohmann/json.hpp>

#include <clp/ReaderInterface.hpp>

#include <clp_ffi_js/ir/StreamReader.hpp>

namespace clp_ffi_js::ir {
/**
 * Rewinds the reader to the beginning then validates the CLP IR data encoding type.
 *
 * @param reader
 * @throws ClpFfiJsException if the encoding type couldn't be decoded or the encoding type is
 * unsupported.
 */
auto rewind_reader_and_validate_encoding_type(clp::ReaderInterface& reader) -> void;

/**
 * Deserializes the metadata from the IR stream's preamble.
 *
 * @param reader
 * @throws ClpFfiJsException if the preamble couldn't be deserialized.
 * @return The IR stream's metadata as a JSON object.
 */
[[nodiscard]] auto deserialize_metadata(clp::ReaderInterface& reader) -> nlohmann::json;

/**
 * Converts the metadata from the given JSON object to a JavaScript object.
 *
 * @param metadata
 * @return The converted JavaScript object.
 */
[[nodiscard]] auto convert_metadata_to_js_object(nlohmann::json const& metadata) -> MetadataTsType;
}  // namespace clp_ffi_js::ir

#endif  // CLP_FFI_JS_IR_DECODING_METHODS_HPP
