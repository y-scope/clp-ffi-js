#ifndef CLP_FFI_JS_UTILS_HPP
#define CLP_FFI_JS_UTILS_HPP

#include <string>

#include <nlohmann/json.hpp>

namespace clp_ffi_js {
/**
 * @see nlohmann::basic_json::dump

 * Serializes a JSON value into a string with invalid UTF-8 sequences replaced rather than throwing
 * an exception.
 * @param json_obj
 * @return The JSON object serialized as a string.
 */
[[nodiscard]] auto dump_json_with_replace(nlohmann::json const& json_obj) -> std::string;
}  // namespace clp_ffi_js
#endif  // CLP_FFI_JS_UTILS_HPP
