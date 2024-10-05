#ifndef CLP_FFI_JS_IR_LOG_EVENT_WITH_LEVEL_HPP
#define CLP_FFI_JS_IR_LOG_EVENT_WITH_LEVEL_HPP

#include <clp/ir/LogEvent.hpp>
#include <utility>

namespace clp_ffi_js::ir {

/**
 * A class derived from LogEvent with an additional member for log level.
 * @tparam encoded_variable_t The type of encoded variables in the event
 */
template <typename encoded_variable_t>
class LogEventWithLevel : public clp::ir::LogEvent<encoded_variable_t> {
public:
    // Constructors
    LogEventWithLevel(
            clp::ir::epoch_time_ms_t timestamp,
            clp::UtcOffset utc_offset,
            clp::ir::EncodedTextAst<encoded_variable_t> message,
            size_t log_level
    )
            : clp::ir::LogEvent<encoded_variable_t>(timestamp, utc_offset, std::move(message)),
              m_log_level{log_level} {}

    // Methods
    [[nodiscard]] auto get_log_level() const -> size_t;

private:
    size_t m_log_level;
};

template <typename encoded_variable_t>
auto LogEventWithLevel<encoded_variable_t>::get_log_level() const -> size_t {
    return m_log_level;
}
}  // namespace clp_ffi_js::ir

#endif   // CLP_FFI_JS_IR_LOG_EVENT_WITH_LEVEL_HPP
