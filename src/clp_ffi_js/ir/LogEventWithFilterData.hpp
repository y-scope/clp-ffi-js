#ifndef CLP_FFI_JS_IR_LOGEVENTWITHFILTERDATA_HPP
#define CLP_FFI_JS_IR_LOGEVENTWITHFILTERDATA_HPP

#include <utility>

#include <clp/ir/types.hpp>

#include <clp_ffi_js/constants.hpp>

namespace clp_ffi_js::ir {
/**
 * A templated class that extends a log event type with processed versions of some of its fields,
 * specifically the fields that are used for filtering in the `StreamReader` classes and their
 * callers.
 *
 * @tparam LogEvent The type of the log event.
 */
template <typename LogEvent>
class LogEventWithFilterData {
public:
    // Constructor
    LogEventWithFilterData(
            LogEvent log_event,
            LogLevel log_level,
            clp::ir::epoch_time_ms_t timestamp
    )
            : m_log_event{std::move(log_event)},
              m_log_level{log_level},
              m_timestamp{timestamp} {}

    // Disable copy constructor and assignment operator
    LogEventWithFilterData(LogEventWithFilterData const&) = delete;
    auto operator=(LogEventWithFilterData const&) -> LogEventWithFilterData& = delete;

    // Default move constructor and assignment operator
    LogEventWithFilterData(LogEventWithFilterData&&) = default;
    auto operator=(LogEventWithFilterData&&) -> LogEventWithFilterData& = default;

    // Destructor
    ~LogEventWithFilterData() = default;

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
