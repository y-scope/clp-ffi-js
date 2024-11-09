#ifndef CLP_FFI_JS_IR_LOGEVENTWITHFILTERDATA_HPP
#define CLP_FFI_JS_IR_LOGEVENTWITHFILTERDATA_HPP

#include <utility>

#include <clp/ir/EncodedTextAst.hpp>
#include <clp/ir/LogEvent.hpp>
#include <clp/ir/types.hpp>
#include <clp/time_types.hpp>

#include <clp_ffi_js/constants.hpp>

namespace clp_ffi_js::ir {
/**
 * A class which accepts a log event type as a template parameter and provides additional members
 * for the log level and timestamp. The additional members facilitate log level filtering.
 * @tparam LogEvent The type of the log event.
 */
template <typename LogEvent>
class LogEventWithFilterData {
public:
    // Constructor
    explicit LogEventWithFilterData(LogEvent log_event, LogLevel log_level, clp::ir::epoch_time_ms_t timestamp)
        : m_log_event{std::move(log_event)},
          m_log_level{log_level},
          m_timestamp{timestamp} {}

    [[nodiscard]] auto get_log_event() const -> LogEvent const& { return m_log_event; }

    [[nodiscard]] auto get_log_level() const -> LogLevel { return m_log_level; }

    [[nodiscard]] auto get_timestamp() const -> clp::ir::epoch_time_ms_t { return m_timestamp; }

private:
    LogEvent m_log_event;
    LogLevel m_log_level;
    clp::ir::epoch_time_ms_t m_timestamp;
};
}  // namespace clp_ffi_js::ir

#endif  // CLP_FFI_JS_IR_LOGEVENTWITHFILTERDATA_HPP