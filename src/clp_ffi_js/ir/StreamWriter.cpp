#include <emscripten/bind.h>
#include <emscripten/val.h>

#include <clp_ffi_js/ir/StreamWriter.hpp>
#include <clp_ffi_js/ir/StructuredIrStreamWriter.hpp>

namespace {
EMSCRIPTEN_BINDINGS(ClpStreamWriter) {
    // JS types used as inputs

    // JS types used as outputs
    emscripten::class_<clp_ffi_js::ir::StreamWriter>("StreamWriter")
            .constructor(
                    &clp_ffi_js::ir::StreamWriter::create,
                    emscripten::return_value_policy::take_ownership()
            )
            .function("write", &clp_ffi_js::ir::StreamWriter::write)
            .function("close", &clp_ffi_js::ir::StreamWriter::close);
}
}  // namespace

namespace clp_ffi_js::ir {
auto StreamWriter::create(emscripten::val stream) -> std::unique_ptr<StreamWriter> {
    return std::make_unique<StructuredIrStreamWriter>(stream);
}
}  // namespace clp_ffi_js::ir
