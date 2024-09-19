#ifndef CLP_FFI_JS_IR_STREAM_READER_HPP
#define CLP_FFI_JS_IR_STREAM_READER_HPP

#include <cstddef>
#include <memory>
#include <vector>

#include <clp/ir/LogEvent.hpp>
#include <clp/ir/types.hpp>
#include <clp/TimestampPattern.hpp>
#include <emscripten/bind.h>
#include <emscripten/val.h>

#include <clp_ffi_js/ir/StreamReaderDataContext.hpp>
#include <clp_ffi_js/ir/LogViewerEvent.hpp>

namespace clp_ffi_js::ir {
EMSCRIPTEN_DECLARE_VAL_TYPE(DataArrayTsType);
EMSCRIPTEN_DECLARE_VAL_TYPE(DecodedResultsTsType);
EMSCRIPTEN_DECLARE_VAL_TYPE(FilterIndicesType);

/**
 * Class to deserialize and decode Zstandard-compressed CLP IR streams as well as format decoded
 * log events.
 */
class StreamReader {
public:
    /**
     * Creates a StreamReader to read from the given array.
     *
     * @param data_array An array containing a Zstandard-compressed IR stream.
     * @return The created instance.
     * @throw ClpFfiJsException if any error occurs.
     */
    [[nodiscard]] static auto create(DataArrayTsType const& data_array) -> StreamReader;

    // Destructor
    ~StreamReader() = default;

    // Disable copy constructor and assignment operator
    StreamReader(StreamReader const&) = delete;
    auto operator=(StreamReader const&) -> StreamReader& = delete;

    // Define default move constructor
    StreamReader(StreamReader&&) = default;
    // Delete move assignment operator since it's also disabled in `clp::ir::LogEventDeserializer`.
    auto operator=(StreamReader&&) -> StreamReader& = delete;

    /**
     * @return The number of events buffered.
     */
    [[nodiscard]] auto get_num_events_buffered() const -> size_t;

        /**
     * @return The number of events buffered.
     */
    [[nodiscard]] auto get_filtered_log_indices() const -> FilterIndicesType;

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
    [[nodiscard]] auto deserialize_range(size_t begin_idx, size_t end_idx) -> size_t;

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
    [[nodiscard]] auto decode_range(size_t begin_idx, size_t end_idx) const -> DecodedResultsTsType;

    /**
     * Creates an array containing indexes of logs which match the user selected levels. The
     * previous array is removed and a new one is created on each call.
     *
     * @param logLevelFilter Array of selected log levels
     */
    void filter_logs(const emscripten::val& logLevelFilter);

    //member function to create a vector of indices
    [[nodiscard]] static auto create_indices_vector(int length) -> std::vector<size_t>;

private:
    // Constructor
    explicit StreamReader(StreamReaderDataContext<clp::ir::four_byte_encoded_variable_t>&&
                                  stream_reader_data_context);

    // Variables
    std::vector<clp::ffi::js::LogViewerEvent<clp::ir::four_byte_encoded_variable_t>> m_encoded_log_events;
    std::vector<size_t> m_filtered_log_event_indices;
    std::unique_ptr<StreamReaderDataContext<clp::ir::four_byte_encoded_variable_t>>
            m_stream_reader_data_context;
    clp::TimestampPattern m_ts_pattern;
};
}  // namespace clp_ffi_js::ir

#endif  // CLP_FFI_JS_IR_STREAM_READER_HPP
