#ifndef CLP_FFI_JS_JSON_UTILS_HPP
#define CLP_FFI_JS_JSON_UTILS_HPP

#include <string>

#include <json/single_include/nlohmann/json.hpp>

namespace clp_ffi_js {
/**
 * @see nlohmann::basic_json::dump
 * Serializes a JSON value into a string with invalid UTF-8 sequences replaced rather than
 * throwing an exception.
 * @param json
 * @return Serialized JSON.
 */
[[nodiscard]] inline auto dump_json_with_replace(nlohmann::json const& json) -> std::string {
    return json.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
}
}  // namespace clp_ffi_js
#endif  // CLP_FFI_JS_JSON_UTILS_HPP
