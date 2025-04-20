#ifndef CLP_FFI_JS_IR_STRUCTUREDIRSTREAMWRITER_HPP
#define CLP_FFI_JS_IR_STRUCTUREDIRSTREAMWRITER_HPP

namespace clp_ffi_js::ir
{
    class StructuredIrStreamWriter : public StreamWriter
    {
    public:
        // Delete default constructor to disable direct instantiation.
        StructuredIrStreamWriter() = delete;

        // Delete copy & move constructors and assignment operators
        StructuredIrStreamWriter(StructuredIrStreamWriter const&) = delete;
        StructuredIrStreamWriter(StructuredIrStreamWriter&&) = delete;
        auto operator=(StructuredIrStreamWriter const&) -> StructuredIrStreamWriter& = delete;
        auto operator=(StructuredIrStreamWriter&&) -> StructuredIrStreamWriter& = delete;

        // Destructor
        ~StructuredIrStreamWriter() override = default;

        StructuredIrStreamWriter(emscripten::val stream);
        auto write(::emscripten::val chunk) -> void override;
        auto flush() -> void override;
        auto close() -> void override;

    private:
        [[nodiscard]] auto write_ir_buf_to_output_stream() -> bool;

        // Variables
        emscripten::val m_output_writer;
        std::unique_ptr<ClpIrSerializer> m_serializer;
    };
} // clp_ffi_js::ir

#endif // CLP_FFI_JS_IR_STRUCTUREDIRSTREAMWRITER_HPP
