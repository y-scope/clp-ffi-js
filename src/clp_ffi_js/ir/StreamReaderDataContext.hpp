#ifndef CLP_FFI_JS_IR_STREAMREADERDATACONTEXT_HPP
#define CLP_FFI_JS_IR_STREAMREADERDATACONTEXT_HPP

#include <memory>
#include <utility>

#include <clp/Array.hpp>
#include <clp/streaming_compression/zstd/Decompressor.hpp>
#include <ffi/ir_stream/Deserializer.hpp>

namespace clp_ffi_js::ir {
/**
 * The data context for a `StreamReader`. It encapsulates a chain of the following resources:
 * A `clp::ir::LogEventDeserializer` that reads from a
 * `clp::streaming_compression::zstd::Decompressor`, which in turn reads from a `clp::Array`.
 */
class StreamReaderDataContext {
public:
    // Constructors
    StreamReaderDataContext(
            clp::Array<char>&& data_buffer,
            std::unique_ptr<clp::streaming_compression::zstd::Decompressor>&& zstd_decompressor,
            clp::ffi::ir_stream::Deserializer deserializer
    )
            : m_data_buffer{std::move(data_buffer)},
              m_zstd_decompressor{std::move(zstd_decompressor)},
              m_deserializer{std::move(deserializer)} {}

    // Disable copy constructor and assignment operator
    StreamReaderDataContext(StreamReaderDataContext const&) = delete;
    auto operator=(StreamReaderDataContext const&) -> StreamReaderDataContext& = delete;

    // Default move constructor and assignment operator
    StreamReaderDataContext(StreamReaderDataContext&&) = default;
    auto operator=(StreamReaderDataContext&&) -> StreamReaderDataContext& = default;

    // Destructor
    ~StreamReaderDataContext() = default;

    // Methods
    /**
     * @return A reference to the reader.
     */
    [[nodiscard]] auto get_reader() const -> clp::streaming_compression::zstd::Decompressor& {
        return *m_zstd_decompressor;
    }

    /**
     * @return A reference to the deserializer.
     */
    [[nodiscard]] auto get_deserializer() -> clp::ffi::ir_stream::Deserializer& {
        return m_deserializer;
    }

private:
    clp::Array<char> m_data_buffer;
    std::unique_ptr<clp::streaming_compression::zstd::Decompressor> m_zstd_decompressor;
    clp::ffi::ir_stream::Deserializer m_deserializer;
};
}  // namespace clp_ffi_js::ir

#endif  // CLP_FFI_JS_IR_STREAMREADERDATACONTEXT_HPP
