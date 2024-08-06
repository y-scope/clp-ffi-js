#ifndef CLP_FFI_JS_IR_CLPIRV1DECODER_HPP
#define CLP_FFI_JS_IR_CLPIRV1DECODER_HPP

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
class ClpIrV1Decoder {
public:
    /**
     * Creates a StreamReader to read from the given array.
     *
     * @param data_array An array containing a Zstandard-compressed IR stream.
     * @return The created instance.
     * @throw ClpJsException if any error occurs.
     */
    [[nodiscard]] static auto create(emscripten::val const& data_array) -> ClpIrV1Decoder;

    // Destructor
    ~ClpIrV1Decoder() = default;

    // Explicitly disable copy constructor and assignment operator
    ClpIrV1Decoder(ClpIrV1Decoder const&) = delete;
    auto operator=(ClpIrV1Decoder const&) -> ClpIrV1Decoder& = delete;

    // Define default move constructor
    ClpIrV1Decoder(ClpIrV1Decoder&&) = default;
    // Delete move assignment operator since it's also disabled in `clp::ir::LogEventDeserializer`.
    auto operator=(ClpIrV1Decoder&&) -> ClpIrV1Decoder& = delete;

    /**
     * Retrieves an estimated number of log events.
     *
     * @return The number of log events deserialized if `build_idx()` has been called and returned
     * successfully.
     * @return 0 (cFullRangeEndIdx) to indicate there is no event has been deserialized yet by
     * `build_idx()`.
     */
    [[nodiscard]] auto get_estimated_num_events() const -> size_t;

    /**
     * Deserializes log events in the range `[beginIdx, endIdx)` and buffers the results. Currently,
     * only full range build is supported, and the buffered bytes of the stream will be released on
     * success.
     *
     * @param begin_idx
     * @param end_idx
     * @return Count of the successfully deserialized ("valid") log events and count of any
     * un-deserializable ("invalid") log events within the range
     * @return null if any log event in the range does not exist (e.g., the range exceeds the number
     * of log events in the file).
     */
    [[nodiscard]] auto build_idx(size_t begin_idx, size_t end_idx) -> emscripten::val;

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
    explicit ClpIrV1Decoder(
            std::unique_ptr<char const[]>&& data_buffer,
            std::shared_ptr<clp::streaming_compression::zstd::Decompressor> zstd_decompressor,
            clp::ir::LogEventDeserializer<clp::ir::four_byte_encoded_variable_t> deserializer
    );

    // Variables
    bool m_full_range_built{false};
    std::vector<clp::ir::LogEvent<clp::ir::four_byte_encoded_variable_t>> m_log_events;

    std::unique_ptr<char const[]> m_data_buffer;
    std::shared_ptr<clp::streaming_compression::zstd::Decompressor> m_zstd_decompressor;
    clp::ir::LogEventDeserializer<clp::ir::four_byte_encoded_variable_t> m_deserializer;
    clp::TimestampPattern m_ts_pattern;
};
}  // namespace clp_ffi_js::ir

#endif  // CLP_FFI_JS_IR_CLPIRV1DECODER_HPP
