#ifndef CLP_FFI_JS_IR_STREAM_READER_CONTEXT_HPP
#define CLP_FFI_JS_IR_STREAM_READER_CONTEXT_HPP

#include <memory>
#include <utility>

#include <clp/Array.hpp>
#include <clp/ir/LogEventDeserializer.hpp>
#include <clp/ir/types.hpp>
#include <clp/streaming_compression/zstd/Decompressor.hpp>

namespace clp_ffi_js::ir {
/**
 * The data context for a `StreamReader`. It encapsulates a chain of the following resources:
 * A `clp::ir::LogEventDeserializer` that reads from a
 * `clp::streaming_compression::zstd::Decompressor`, which in turn reads from a `clp::Array`.
 * @tparam encoded_variable_t Type of encoded variables encoded in the stream.
 */
template <typename encoded_variable_t>
class StreamReaderContext {
public:
    // Constructors
    explicit StreamReaderContext(
            clp::Array<char>&& data_buffer,
            std::unique_ptr<clp::streaming_compression::zstd::Decompressor>&& zstd_decompressor,
            clp::ir::LogEventDeserializer<clp::ir::four_byte_encoded_variable_t> deserializer
    )
            : m_data_buffer{std::make_unique<clp::Array<char>>(std::move(data_buffer))},
              m_zstd_decompressor{std::move(zstd_decompressor)},
              m_deserializer{std::move(deserializer)} {}

    // Disable copy constructor and assignment operator
    StreamReaderContext(StreamReaderContext const&) = delete;
    auto operator=(StreamReaderContext const&) -> StreamReaderContext& = delete;

    // Default move constructor and assignment operator
    StreamReaderContext(StreamReaderContext&&) = default;
    auto operator=(StreamReaderContext&&) -> StreamReaderContext& = default;

    // Destructor
    ~StreamReaderContext() = default;

    // Methods
    /**
     * @return A reference to the deserializer.
     */
    [[nodiscard]] auto get_deserializer() -> clp::ir::LogEventDeserializer<encoded_variable_t>& {
        return m_deserializer;
    }

private:
    std::unique_ptr<clp::Array<char>> m_data_buffer;
    std::unique_ptr<clp::streaming_compression::zstd::Decompressor> m_zstd_decompressor;
    clp::ir::LogEventDeserializer<encoded_variable_t> m_deserializer;
};
}  // namespace clp_ffi_js::ir

#endif  // CLP_FFI_JS_IR_STREAM_READER_CONTEXT_HPP
