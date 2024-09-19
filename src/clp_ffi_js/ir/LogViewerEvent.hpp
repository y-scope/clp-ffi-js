#ifndef CLP_IR_LOG_VIEWER_EVENT_HPP
#define CLP_IR_LOG_VIEWER_EVENT_HPP

#include <clp/ir/LogEvent.hpp>
#include <array>
#include <string_view>
#include <utility>

namespace clp::ffi::js {

/**
 * A class representing a log event with an additional log level field.
 * @tparam encoded_variable_t The type of encoded variables in the event
 */
template <typename encoded_variable_t>
class LogViewerEvent : public ir::LogEvent<encoded_variable_t> {
public:
    // Constructors
    LogViewerEvent(
            ir::epoch_time_ms_t timestamp,
            UtcOffset utc_offset,
            ir::EncodedTextAst<encoded_variable_t> message,
            size_t log_level // Add the log level parameter
    )
            : ir::LogEvent<encoded_variable_t>(timestamp, utc_offset, std::move(message)),
              m_log_level{log_level} {}

    // Methods
    [[nodiscard]] auto get_log_level() const -> size_t;

    // Static method to create LogViewerEvent from LogEvent
    static auto from_log_event(
            const ir::LogEvent<encoded_variable_t>& log_event,
            size_t log_level
    ) -> LogViewerEvent;

private:
    size_t m_log_level;
};

template <typename encoded_variable_t>
auto LogViewerEvent<encoded_variable_t>::get_log_level() const -> size_t {
    return m_log_level;
}

template <typename encoded_variable_t>
auto LogViewerEvent<encoded_variable_t>::from_log_event(
        const ir::LogEvent<encoded_variable_t>& log_event,
        size_t log_level
) -> LogViewerEvent {
    return LogViewerEvent(
        log_event.get_timestamp(),
        log_event.get_utc_offset(),
        log_event.get_message(),
        log_level
    );
}

}  // namespace clp::ffi:js

#endif  // CLP_IR_LOG_VIEWER_EVENT_HPP