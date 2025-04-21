#ifndef CLP_FFI_JS_IR_STREAMWRITER_HPP
#define CLP_FFI_JS_IR_STREAMWRITER_HPP

#include <clp/ffi/ir_stream/Serializer.hpp>
#include <clp/ir/types.hpp>
#include <emscripten/val.h>

namespace clp_ffi_js::ir {
class StreamWriter {
public:
    using ClpIrSerializer = clp::ffi::ir_stream::Serializer<clp::ir::four_byte_encoded_variable_t>;
    using BufferView = ClpIrSerializer::BufferView;

    /**
     * Creates a `StreamWriter` to read from the given array.
     *
     * @return The created instance.
     * @throw ClpFfiJsException if any error occurs.
     */
    [[nodiscard]] static auto create(emscripten::val stream) -> std::unique_ptr<StreamWriter>;

    // Delete copy & move constructors and assignment operators
    StreamWriter(StreamWriter const&) = delete;
    StreamWriter(StreamWriter&&) = delete;
    auto operator=(StreamWriter const&) -> StreamWriter& = delete;
    auto operator=(StreamWriter&&) -> StreamWriter& = delete;

    // Destructor
    virtual ~StreamWriter() = default;

    /**
     * FIXME: consider separation.
     * Writes a passed chunk of data to a WritableStream and its underlying sink.
     */
    virtual auto write(emscripten::val chunk) -> void = 0;

    /**
     * FIXME: look into integrating this with `WritableStreamDefaultWriter.ready`
     * Flushes the underlying IR buffer and `m_output_stream`.
     */
    virtual auto flush() -> void = 0;

    /**
     * Closes the serializer by writing the buffered results into the output
     * stream with end-of-stream IR Unit appended in the end.
     * @return true on success.
     * @return false on failure.
     */
    virtual auto close() -> void = 0;

protected:
    // TODO: add docs
    StreamWriter() = default;
};
}  // namespace clp_ffi_js::ir

#endif  // CLP_FFI_JS_IR_STREAMWRITER_HPP
