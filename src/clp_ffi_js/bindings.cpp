#include <emscripten/bind.h>
#include "clp_ffi_js/ir/StreamReader.hpp"
#include "clp_ffi_js/ir/KVPairIRStreamReader.hpp"
#include "clp_ffi_js/ir/IrStreamReader.hpp"

namespace {
    EMSCRIPTEN_BINDINGS(ClpIrStreamReader) {
        emscripten::register_type<clp_ffi_js::ir::DataArrayTsType>("Uint8Array");
        emscripten::register_type<clp_ffi_js::ir::DecodedResultsTsType>(
                "Array<[string, number, number, number]>"
        );
        emscripten::register_type<clp_ffi_js::ir::FilteredLogEventMapTsType>("number[] | null");

        emscripten::class_<clp_ffi_js::ir::IrStreamReader,
                emscripten::base<clp_ffi_js::ir::StreamReader>>("ClpIrStreamReader")
                .constructor(
                        &clp_ffi_js::ir::IrStreamReader::create,
                        emscripten::return_value_policy::take_ownership()
                )
                .function(
                        "getNumEventsBuffered",
                        &clp_ffi_js::ir::IrStreamReader::get_num_events_buffered
                )
                .function(
                        "getFilteredLogEventMap",
                        &clp_ffi_js::ir::IrStreamReader::get_filtered_log_event_map
                )
                .function("filterLogEvents", &clp_ffi_js::ir::IrStreamReader::filter_log_events)
                .function("deserializeStream", &clp_ffi_js::ir::IrStreamReader::deserialize_stream)
                .function("decodeRange", &clp_ffi_js::ir::IrStreamReader::decode_range);

        emscripten::class_<clp_ffi_js::ir::KVPairIRStreamReader,
                emscripten::base<clp_ffi_js::ir::StreamReader>>("ClpKVPairIRStreamReader")
                .constructor(
                        &clp_ffi_js::ir::KVPairIRStreamReader::create,
                        emscripten::return_value_policy::take_ownership()
                )
                .function(
                        "getNumEventsBuffered",
                        &clp_ffi_js::ir::KVPairIRStreamReader::get_num_events_buffered
                )
                .function(
                        "deserializeStream",
                        &clp_ffi_js::ir::KVPairIRStreamReader::deserialize_stream
                )
                .function("decodeRange", &clp_ffi_js::ir::KVPairIRStreamReader::decode_range);

        emscripten::class_<clp_ffi_js::ir::StreamReader>("ClpStreamReader")
                .constructor(
                        &clp_ffi_js::ir::StreamReader::create,
                        emscripten::return_value_policy::take_ownership()
                );
    }
}  // namespace
