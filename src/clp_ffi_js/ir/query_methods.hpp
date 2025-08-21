#ifndef CLP_FFI_JS_IR_QUERY_METHODS_HPP
#define CLP_FFI_JS_IR_QUERY_METHODS_HPP

#include <vector>

#include <clp/ReaderInterface.hpp>
#include <ystdlib/containers/Array.hpp>

namespace clp_ffi_js::ir {
/**
 * This function searches through the log events in the IR stream provided by the `reader` for
 * events that match the given `query_string`. It returns a vector of `size_t` representing the
 * indices of the log events that satisfy the query.
 *
 * @param reader An interface for reading log events.
 * @param query_string The query string to match against log events.
 * @return A vector of indices of the log events that matched the query.
 * @throws ClpFfiJsException if the preamble couldn't be deserialized or the Query couldn't be
 * executed.
 */
auto
collect_matched_log_event_indices(clp::ReaderInterface& reader, std::string const& query_string)
        -> std::vector<size_t>;
}  // namespace clp_ffi_js::ir

#endif  // CLP_FFI_JS_IR_QUERY_METHODS_HPP
