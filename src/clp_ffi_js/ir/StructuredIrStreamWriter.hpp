#ifndef CLP_FFI_JS_IR_STRUCTUREDIRSTREAMWRITER_HPP
#define CLP_FFI_JS_IR_STRUCTUREDIRSTREAMWRITER_HPP

#include <clp/streaming_compression/zstd/Compressor.hpp>
#include <clp/WriterInterface.hpp>

namespace clp_ffi_js::ir {
class StructuredIrStreamWriter : public StreamWriter {
public:
    /**
     * The default Msgpack buffer size limit.
     */
    static constexpr size_t cDefaultMsgpackBufferSizeLimit{4096};

    /**
     * The default IR buffer size limit.
     */
    static constexpr size_t cDefaultIrBufferSizeLimit{65'536};

    // Delete default constructor to disable direct instantiation.
    StructuredIrStreamWriter() = delete;

    // Delete copy & move constructors and assignment operators
    StructuredIrStreamWriter(StructuredIrStreamWriter const&) = delete;
    StructuredIrStreamWriter(StructuredIrStreamWriter&&) = delete;
    auto operator=(StructuredIrStreamWriter const&) -> StructuredIrStreamWriter& = delete;
    auto operator=(StructuredIrStreamWriter&&) -> StructuredIrStreamWriter& = delete;

    // Destructor
    ~StructuredIrStreamWriter() override = default;

    StructuredIrStreamWriter(emscripten::val const& stream);
    auto write(::emscripten::val chunk) -> void override;
    auto flush() -> void override;
    auto close() -> void override;

private:
    auto write_ir_buf_to_output_stream() const -> void;

    [[nodiscard]] auto get_ir_buf_size() const -> size_t {
        return m_serializer->get_ir_buf_view().size();
    }

    // Variables
    std::unique_ptr<clp::WriterInterface> m_output_writer;
    std::unique_ptr<clp::streaming_compression::zstd::Compressor> m_writer;
    std::unique_ptr<ClpIrSerializer> m_serializer;

    std::vector<u_int8_t> m_msgpack_buf;
    size_t m_num_total_bytes_serialized;
};
}  // namespace clp_ffi_js::ir

#endif  // CLP_FFI_JS_IR_STRUCTUREDIRSTREAMWRITER_HPP
