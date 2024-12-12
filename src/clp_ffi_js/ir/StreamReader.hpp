#ifndef CLP_FFI_JS_IR_STREAMREADER_HPP
#define CLP_FFI_JS_IR_STREAMREADER_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include <clp/streaming_compression/zstd/Decompressor.hpp>
#include <emscripten/val.h>

namespace clp_ffi_js::ir {
// JS types used as inputs
EMSCRIPTEN_DECLARE_VAL_TYPE(DataArrayTsType);
EMSCRIPTEN_DECLARE_VAL_TYPE(LogLevelFilterTsType);
EMSCRIPTEN_DECLARE_VAL_TYPE(ReaderOptions);

// JS types used as outputs
EMSCRIPTEN_DECLARE_VAL_TYPE(DecodedResultsTsType);
EMSCRIPTEN_DECLARE_VAL_TYPE(FilteredLogEventMapTsType);

enum class StreamType : uint8_t {
    Structured,
    Unstructured,
};

/**
 * Mapping between an index in the filtered log events collection to an index in the unfiltered
 * log events collection.
 */
using FilteredLogEventsMap = std::optional<std::vector<size_t>>;

/**
 * Class to deserialize and decode Zstandard-compressed CLP IR streams as well as format decoded
 * log events.
 */
class StreamReader {
public:
    using ZstdDecompressor = clp::streaming_compression::zstd::Decompressor;

    /**
     * Creates a `StreamReader` to read from the given array.
     *
     * @param data_array An array containing a Zstandard-compressed IR stream.
     * @param reader_options
     * @return The created instance.
     * @throw ClpFfiJsException if any error occurs.
     */
    [[nodiscard]] static auto create(
            DataArrayTsType const& data_array,
            ReaderOptions const& reader_options
    ) -> std::unique_ptr<StreamReader>;

    // Destructor
    virtual ~StreamReader() = default;

    // Disable copy constructor and assignment operator
    StreamReader(StreamReader const&) = delete;
    auto operator=(StreamReader const&) -> StreamReader& = delete;

    // Define default move constructor
    StreamReader(StreamReader&&) = default;
    // Delete move assignment operator since it's also disabled in `clp::ir::LogEventDeserializer`.
    auto operator=(StreamReader&&) -> StreamReader& = delete;

    // Methods
    [[nodiscard]] virtual auto get_ir_stream_type() const -> StreamType = 0;

    /**
     * @return The number of events buffered.
     */
    [[nodiscard]] virtual auto get_num_events_buffered() const -> size_t = 0;

    /**
     * @return The filtered log events map.
     */
    [[nodiscard]] virtual auto get_filtered_log_event_map() const -> FilteredLogEventMapTsType = 0;

    /**
     * Generates a filtered collection from all log events.
     *
     * @param log_level_filter Array of selected log levels
     */
    virtual void filter_log_events(LogLevelFilterTsType const& log_level_filter) = 0;

    /**
     * Deserializes all log events in the stream.
     *
     * @return The number of successfully deserialized ("valid") log events.
     */
    [[nodiscard]] virtual auto deserialize_stream() -> size_t = 0;

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
    [[nodiscard]] virtual auto decode_range(size_t begin_idx, size_t end_idx, bool use_filter) const
            -> DecodedResultsTsType = 0;

protected:
    explicit StreamReader() = default;
};
}  // namespace clp_ffi_js::ir

#endif  // CLP_FFI_JS_IR_STREAMREADER_HPP
