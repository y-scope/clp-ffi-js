#ifndef CLP_FFI_JS_IR_STREAM_READER_HPP
#define CLP_FFI_JS_IR_STREAM_READER_HPP

#include <Array.hpp>
#include <cstddef>
#include <memory>
#include <optional>
#include <streaming_compression/zstd/Decompressor.hpp>
#include <vector>

#include <clp/ir/types.hpp>
#include <clp/TimestampPattern.hpp>
#include <emscripten/val.h>

#include <clp_ffi_js/ir/LogEventWithLevel.hpp>
#include <clp_ffi_js/ir/StreamReaderDataContext.hpp>

using clp::ir::four_byte_encoded_variable_t;

namespace clp_ffi_js::ir {
EMSCRIPTEN_DECLARE_VAL_TYPE(DataArrayTsType);
EMSCRIPTEN_DECLARE_VAL_TYPE(DecodedResultsTsType);
EMSCRIPTEN_DECLARE_VAL_TYPE(FilteredLogEventMapTsType);

/**
 * Class to deserialize and decode Zstandard-compressed CLP IR streams as well as format decoded
 * log events.
 */
class StreamReader {
public:
    /**
     * Mapping between an index in the filtered log events collection to an index in the unfiltered
     * log events collection.
     */
    using FilteredLogEventsMap = std::optional<std::vector<size_t>>;

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

    // Methods
    /**
     * @return The number of events buffered.
     */
    [[nodiscard]] auto get_num_events_buffered() const -> size_t;

    /**
     * @return The filtered log events map.
     */
    [[nodiscard]] auto get_filtered_log_event_map() const -> FilteredLogEventMapTsType;

    /**
     * Generates a filtered collection from all log events.
     *
     * @param log_level_filter Array of selected log levels
     */
    void filter_log_events(emscripten::val const& log_level_filter);

    /**
     * Deserializes all log events in the stream. After the stream has been exhausted, it will be
     * deallocated.
     *
     * @return The number of successfully deserialized ("valid") log events.
     */
    [[nodiscard]] auto deserialize_stream() -> size_t;

    /**
     * Decodes log events in the range `[beginIdx, endIdx)` of the filtered or unfiltered
     * (depending on the value of `useFilter`) log events collection.
     *
     * @param begin_idx
     * @param end_idx
     * @param use_filter Whether to decode from the filtered or unfiltered log events collection.
     * @return An array where each element is a decoded log event represented by an array of:
     * - The log event's message
     * - The log event's timestamp as milliseconds since the Unix epoch
     * - The log event's log level as an integer that indexes into `cLogLevelNames`
     * - The log event's number (1-indexed) in the stream
     * @return null if any log event in the range doesn't exist (e.g. the range exceeds the number
     * of log events in the collection).
     */
    [[nodiscard]] auto
    decode_range(size_t begin_idx, size_t end_idx, bool use_filter) const -> DecodedResultsTsType;

private:
    // Constructor
    explicit StreamReader(
            StreamReaderDataContext<four_byte_encoded_variable_t>&& stream_reader_data_context
    );

    // Methods
    [[nodiscard]] static auto create_deserializer_and_data_context(
            std::unique_ptr<clp::streaming_compression::zstd::Decompressor>&& zstd_decompressor,
            clp::Array<char>&& data_buffer
    ) -> StreamReaderDataContext<four_byte_encoded_variable_t>;

    // Variables
    std::vector<LogEventWithLevel<four_byte_encoded_variable_t>> m_encoded_log_events;
    std::unique_ptr<StreamReaderDataContext<four_byte_encoded_variable_t>>
            m_stream_reader_data_context;
    FilteredLogEventsMap m_filtered_log_event_map;
    clp::TimestampPattern m_ts_pattern;
};
}  // namespace clp_ffi_js::ir

#endif  // CLP_FFI_JS_IR_STREAM_READER_HPP
