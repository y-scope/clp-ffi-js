#ifndef CLP_FFI_JS_KV_PAIR_IR_STREAM_READER_HPP
#define CLP_FFI_JS_KV_PAIR_IR_STREAM_READER_HPP

#include <cstddef>
#include <ffi/ir_stream/Deserializer.hpp>
#include <memory>
#include <vector>

#include <clp/ffi/KeyValuePairLogEvent.hpp>
#include <emscripten/bind.h>
#include <emscripten/val.h>

#include <clp_ffi_js/ir/LogEventWithLevel.hpp>
#include <clp_ffi_js/ir/StreamReader.hpp>
#include <clp_ffi_js/ir/StreamReaderDataContext.hpp>

namespace clp_ffi_js::ir {

class IrUnitHandler {
public:
    // Implements `clp::ffi::ir_stream::IrUnitHandlerInterface` interface
    [[nodiscard]] auto handle_log_event(clp::ffi::KeyValuePairLogEvent&& log_event) -> clp::ffi::ir_stream::IRErrorCode {
        m_deserialized_log_events.emplace_back(std::move(log_event));
        return clp::ffi::ir_stream::IRErrorCode::IRErrorCode_Success;
    }
    [[nodiscard]] static auto handle_utc_offset_change(
            [[maybe_unused]] clp::UtcOffset utc_offset_old,
            [[maybe_unused]] clp::UtcOffset utc_offset_new
    ) -> clp::ffi::ir_stream::IRErrorCode {
        return clp::ffi::ir_stream::IRErrorCode::IRErrorCode_Success;
    }
    [[nodiscard]] static auto handle_schema_tree_node_insertion(
            [[maybe_unused]] clp::ffi::SchemaTree::NodeLocator schema_tree_node_locator
    ) -> clp::ffi::ir_stream::IRErrorCode {
        return clp::ffi::ir_stream::IRErrorCode::IRErrorCode_Success;
    }
    [[nodiscard]] auto handle_end_of_stream() -> clp::ffi::ir_stream::IRErrorCode {
        m_is_complete = true;
        return clp::ffi::ir_stream::IRErrorCode::IRErrorCode_Success;
    }
    // Methods
    [[nodiscard]] auto is_complete() const -> bool { return m_is_complete; }
    [[nodiscard]] auto get_deserialized_log_events() const -> std::vector<clp::ffi::KeyValuePairLogEvent> const & {
        return m_deserialized_log_events;
    }
private:
    std::vector<clp::ffi::KeyValuePairLogEvent> m_deserialized_log_events;
    bool m_is_complete{false};
};

/**
 * Class to deserialize and decode Zstandard-compressed CLP IR streams as well as format decoded
 * log events.
 */
class KVPairIRStreamReader : public StreamReader {
public:
    /**
     * Creates a StreamReader to read from the given array.
     *
     * @param data_array An array containing a Zstandard-compressed IR stream.
     * @return The created instance.
     * @throw ClpFfiJsException if any error occurs.
     */
    [[nodiscard]] static auto create(DataArrayTsType const& data_array, ReaderOptions const& reader_options) -> KVPairIRStreamReader;

    // Destructor
    ~KVPairIRStreamReader() override = default;

    // Disable copy constructor and assignment operator
    KVPairIRStreamReader(KVPairIRStreamReader const&) = delete;
    auto operator=(KVPairIRStreamReader const&) -> KVPairIRStreamReader& = delete;

    // Define default move constructor
    KVPairIRStreamReader(KVPairIRStreamReader&&) = default;
    // Delete move assignment operator since it's also disabled in `clp::ir::LogEventDeserializer`.
    auto operator=(KVPairIRStreamReader&&) -> KVPairIRStreamReader& = delete;

    /**
     * @return The number of events buffered.
     */
    [[nodiscard]] auto get_num_events_buffered() const -> size_t override;

    [[nodiscard]] auto get_filtered_log_event_map() const -> FilteredLogEventMapTsType override;

    auto filter_log_events(emscripten::val const& log_level_filter) -> void override;
    /**
     * Deserializes and buffers log events in the range `[beginIdx, endIdx)`. After the stream has
     * been exhausted, it will be deallocated.
     *
     * NOTE: Currently, this class only supports deserializing the full range of log events in the
     * stream.
     *
     * @param begin_idx
     * @param end_idx
     * @return The number of successfully deserialized ("valid") log events.
     */
    [[nodiscard]] auto deserialize_stream() -> size_t override;

    /**
     * Decodes the deserialized log events in the range `[beginIdx, endIdx)`.
     *
     * @param begin_idx
     * @param end_idx
     * @return An array where each element is a decoded log event represented by an array of:
     * - The log event's message
     * - The log event's timestamp as milliseconds since the Unix epoch
     * - The log event's log level as an integer that indexes into `cLogLevelNames`
     * - The log event's number (1-indexed) in the stream
     * @return null if any log event in the range doesn't exist (e.g., the range exceeds the number
     * of log events in the file).
     */
    [[nodiscard]] auto decode_range(size_t begin_idx, size_t end_idx, bool use_filter) const
            -> DecodedResultsTsType override;

private:
    using deserializer_t = clp::ffi::ir_stream::Deserializer<IrUnitHandler>;

    // Constructor
    explicit KVPairIRStreamReader(
            StreamReaderDataContext<deserializer_t>&& stream_reader_data_context, ReaderOptions const& reader_options
    );

    // Variables
    std::vector<LogEventWithLevel<clp::ffi::KeyValuePairLogEvent>> m_encoded_log_events;
    std::unique_ptr<StreamReaderDataContext<deserializer_t>> m_stream_reader_data_context;

    std::string m_log_level_key;
    std::string m_timestamp_key;
    FilteredLogEventsMap m_filtered_log_event_map;
};
}  // namespace clp_ffi_js::ir

#endif  // CLP_FFI_JS_KV_PAIR_IR_STREAM_READER_HPP
