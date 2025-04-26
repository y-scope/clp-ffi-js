#include <emscripten/bind.h>
#include <emscripten/val.h>

#include <clp_ffi_js/ir/StreamWriter.hpp>
#include <clp_ffi_js/ir/StructuredIrStreamWriter.hpp>

namespace {
EMSCRIPTEN_BINDINGS(ClpStreamWriter) {
    // JS types used as inputs
    emscripten::register_type<clp_ffi_js::ir::WriterOptions>("{compressionLevel: number | undefined}");

    // JS types used as outputs

    emscripten::class_<clp_ffi_js::ir::StreamWriter>("StreamWriter")
            .constructor(
                    &clp_ffi_js::ir::StreamWriter::create,
                    emscripten::return_value_policy::take_ownership()
            )
            .function("write", &clp_ffi_js::ir::StreamWriter::write)
            .function("close", &clp_ffi_js::ir::StreamWriter::close)
            .property("desiredSize", &clp_ffi_js::ir::StreamWriter::get_desired_size);
}
}  // namespace

namespace clp_ffi_js::ir {
auto StreamWriter::create(emscripten::val const& stream, WriterOptions const& writer_options) -> std::unique_ptr<StreamWriter> {
    return std::make_unique<StructuredIrStreamWriter>(stream, writer_options);
}
}  // namespace clp_ffi_js::ir
