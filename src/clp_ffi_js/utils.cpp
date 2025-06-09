#include "utils.hpp"

#include <json/single_include/nlohmann/json.hpp>

namespace clp_ffi_js {
[[nodiscard]] auto dump_json_with_replace(nlohmann::json const& json_obj) -> std::string {
    return json_obj.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
}
}  // namespace clp_ffi_js
