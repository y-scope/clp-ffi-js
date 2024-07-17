#ifndef CLP_FFI_JS_CLPIRV1DECODER_HPP
#define CLP_FFI_JS_CLPIRV1DECODER_HPP

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

namespace clp_ffi_js {
/**
 * Deserializes ZStandard-compressed CLP IR V1 byte streams and formats extracted log events.
 */
class ClpIrV1Decoder {
public:
    /**
     * Factory method to create a ClpIrV1Decoder instance by
     * - Copying an input array into C++ space.
     * - Constructing a Zstd Decompressor instance.
     * - Constructing a LogEventDeserializer by decoding the preamble of the IR stream, which
     * includes the encoding type.
     * - Passing those to the ClpIrV1Decoder() for their life cycles management.
     *
     * @param data_array JS Uint8Array which contains the Zstd-compressed CLP IR V1 bytes.
     * @return the created instance.
     * @throw DecodingException if any error occurs.
     */
    [[nodiscard]] static auto create(emscripten::val const& data_array) -> ClpIrV1Decoder;

    // Destructor
    ~ClpIrV1Decoder() = default;

    // Explicitly disable copy constructor/assignment
    ClpIrV1Decoder(ClpIrV1Decoder const&) = delete;
    auto operator=(ClpIrV1Decoder const&) -> ClpIrV1Decoder& = delete;

    // Define default move constructor
    ClpIrV1Decoder(ClpIrV1Decoder&&) = default;
    // Delete assignment operator due to Emscripten (derived from LLVM)
    // being unable to resolve the move constructor for clp::ir::LogEventDeserializer
    auto operator=(ClpIrV1Decoder&&) -> ClpIrV1Decoder& = delete;

    /**
     * Retrieves an estimated number of log events.
     *
     * @return Before `build_idx()` is called, return cFullRangeEndIdx=0, indicating that there is
     * no event stored in the log.
     * @return After `build_idx()` is called, return the number of log events that have been
     * successfully deserialized.
     */
    [[nodiscard]] auto get_estimated_num_events() const -> size_t;

    /**
     * When applicable, deserializes log events in the range `[beginIdx, endIdx)`.
     *
     * @param beginIdx
     * @param endIdx
     * @return Count of the successfully deserialized ("valid") log events and count of any
     * un-deserializable ("invalid") log events within the range
     * @return null if any log event in the range does not exist (e.g., the range exceeds the number
     * of log events in the file).
     */
    [[nodiscard]] auto build_idx(size_t begin_idx, size_t end_idx) -> emscripten::val;

    /**
     * Decodes the log events in the range `[beginIdx, endIdx)`.
     *
     * @param beginIdx
     * @param endIdx
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
}  // namespace clp_ffi_js

#endif  // CLP_FFI_JS_CLPIRV1DECODER_HPP
