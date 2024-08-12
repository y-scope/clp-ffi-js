#ifndef CLP_FFI_JS_IR_STREAM_READER_HPP
#define CLP_FFI_JS_IR_STREAM_READER_HPP

#include <cstddef>
#include <memory>
#include <vector>

#include <clp/ir/LogEvent.hpp>
#include <clp/ir/LogEventDeserializer.hpp>
#include <clp/ir/types.hpp>
#include <clp/streaming_compression/zstd/Decompressor.hpp>
#include <clp/TimestampPattern.hpp>
#include <emscripten/bind.h>
#include <emscripten/val.h>

namespace clp_ffi_js::ir {
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
     * @throw ClpJsException if any error occurs.
     */
    [[nodiscard]] static auto create(emscripten::val const& data_array) -> StreamReader;

    // Destructor
    ~StreamReader() = default;

    // Explicitly disable copy constructor and assignment operator
    StreamReader(StreamReader const&) = delete;
    auto operator=(StreamReader const&) -> StreamReader& = delete;

    // Define default move constructor
    StreamReader(StreamReader&&) = default;
    // Delete move assignment operator since it's also disabled in `clp::ir::LogEventDeserializer`.
    auto operator=(StreamReader&&) -> StreamReader& = delete;

    /**
     * Retrieves a number of buffered log events.
     *
     * @return The number of log events deserialized if `build_idx()` has been called and returned
     * successfully.
     * @return 0 (cFullRangeEndIdx) to indicate there is no event has been deserialized yet by
     * `build_idx()`.
     */
    [[nodiscard]] auto get_num_events_buffered() const -> size_t;

    /**
     * Deserializes log events in the range `[beginIdx, endIdx)` and buffers the results. Currently,
     * only full range build is supported, and the buffered bytes of the stream will be released on
     * success.
     *
     * @param begin_idx
     * @param end_idx
     * @return Count of the successfully deserialized ("valid") log events.
     */
    [[nodiscard]] auto build_idx(size_t begin_idx, size_t end_idx) -> size_t;

    /**
     * Decodes the deserialized log events in the range `[beginIdx, endIdx)`.
     *
     * @param begin_idx
     * @param end_idx
     * @return The decoded log events on success.
     * @return null if any log event in the range doesn't exist (e.g., the range exceeds the number
     * of log events in the file).
     */
    [[nodiscard]] auto decode(size_t begin_idx, size_t end_idx) const -> emscripten::val;

private:
    // Constructor
    explicit StreamReader(
            std::vector<char>&& data_buffer,
            std::shared_ptr<clp::streaming_compression::zstd::Decompressor> zstd_decompressor,
            clp::ir::LogEventDeserializer<clp::ir::four_byte_encoded_variable_t> deserializer
    );

    // Variables
    bool m_read_complete{false};
    std::vector<clp::ir::LogEvent<clp::ir::four_byte_encoded_variable_t>> m_log_events;

    std::unique_ptr<std::vector<char>> m_data_buffer;
    std::shared_ptr<clp::streaming_compression::zstd::Decompressor> m_zstd_decompressor;
    clp::ir::LogEventDeserializer<clp::ir::four_byte_encoded_variable_t> m_deserializer;
    clp::TimestampPattern m_ts_pattern;
};
}  // namespace clp_ffi_js::ir

#endif  // CLP_FFI_JS_IR_STREAM_READER_HPP
