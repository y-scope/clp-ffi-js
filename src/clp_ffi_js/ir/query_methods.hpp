#ifndef CLP_FFI_JS_IR_QUERY_METHODS_HPP
#define CLP_FFI_JS_IR_QUERY_METHODS_HPP

#include <vector>

#include <clp/ReaderInterface.hpp>
#include <ystdlib/containers/Array.hpp>

namespace clp_ffi_js::ir {
[[nodiscard]] auto query_index(clp::ReaderInterface& reader, std::string const& query_string)
        -> std::vector<size_t>;
}  // namespace clp_ffi_js::ir

#endif  // CLP_FFI_JS_IR_QUERY_METHODS_HPP
