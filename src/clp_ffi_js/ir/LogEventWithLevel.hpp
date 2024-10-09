#ifndef CLP_FFI_JS_IR_LOGEVENTWITHLEVEL_HPP
#define CLP_FFI_JS_IR_LOGEVENTWITHLEVEL_HPP

#include <utility>

#include <clp/ir/EncodedTextAst.hpp>
#include <clp/ir/LogEvent.hpp>
#include <clp/ir/types.hpp>
#include <clp/time_types.hpp>

#include <clp_ffi_js/constants.hpp>

namespace clp_ffi_js::ir {

/**
 * A class derived from `clp::ir::LogEvent` with an additional member for the log level.
 *
 * NOTE: Once we move to the next IR format, this class should no longer be necessary since each
 * IR log event will contain a set of key-value pairs, one of which should be the log level.
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
            LogLevel log_level
    )
            : clp::ir::LogEvent<encoded_variable_t>{timestamp, utc_offset, std::move(message)},
              m_log_level{log_level} {}

    [[nodiscard]] auto get_log_level() const -> LogLevel { return m_log_level; }

private:
    LogLevel m_log_level;
};
}  // namespace clp_ffi_js::ir

#endif  // CLP_FFI_JS_IR_LOGEVENTWITHLEVEL_HPP
