#ifndef CLP_FFI_JS_UTILS_HPP
#define CLP_FFI_JS_UTILS_HPP

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <optional>
#include <string>

#include <nlohmann/json.hpp>

namespace clp_ffi_js {
/**
 * Finds the log event immediately at or before a target timestamp.
 *
 * The log events must be sorted by timestamp in ascending order. If the target timestamp precedes
 * every log event, the first event is returned.
 *
 * @tparam LogEventRange A random-access range whose elements expose `get_timestamp()`.
 * @tparam Timestamp The timestamp type.
 * @param log_events The chronologically ordered log events.
 * @param target_timestamp The timestamp to search for.
 * @return The zero-based log-event index, or `std::nullopt` if `log_events` is empty.
 */
template <typename LogEventRange, typename Timestamp>
[[nodiscard]] auto
find_nearest_log_event_by_timestamp(LogEventRange const& log_events, Timestamp target_timestamp)
        -> std::optional<size_t> {
    if (log_events.empty()) {
        return std::nullopt;
    }

    auto const first_greater_it{std::upper_bound(
            log_events.begin(),
            log_events.end(),
            target_timestamp,
            [](Timestamp timestamp, auto const& log_event) {
                return timestamp < log_event.get_timestamp();
            }
    )};
    if (first_greater_it == log_events.begin()) {
        return 0;
    }

    return static_cast<size_t>(std::distance(log_events.begin(), first_greater_it) - 1);
}

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
