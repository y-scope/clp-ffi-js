#include "StreamReader.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <format>
#include <memory>
#include <utility>

#include <clp/Array.hpp>
#include <clp/ErrorCode.hpp>
#include <clp/streaming_compression/zstd/Decompressor.hpp>
#include <clp/TraceableException.hpp>
#include <emscripten/bind.h>
#include <spdlog/spdlog.h>

#include <clp_ffi_js/ClpFfiJsException.hpp>
#include <clp_ffi_js/ir/decoding_methods.hpp>
#include <clp_ffi_js/ir/IrStreamReader.hpp>

namespace clp_ffi_js::ir {
auto StreamReader::create(DataArrayTsType const& data_array) -> std::unique_ptr<StreamReader> {
    auto const length{data_array["length"].as<size_t>()};
    SPDLOG_INFO("StreamReader::create: got buffer of length={}", length);

    // Copy array from JavaScript to C++.
    clp::Array<char> data_buffer{length};
    // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
    emscripten::val::module_property("HEAPU8")
            .call<void>("set", data_array, reinterpret_cast<uintptr_t>(data_buffer.data()));
    // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)

    auto zstd_decompressor{std::make_unique<clp::streaming_compression::zstd::Decompressor>()};
    zstd_decompressor->open(data_buffer.data(), length);

    auto const version{get_version(*zstd_decompressor)};

    if (std::ranges::find(cIrV1Versions, version) != cIrV1Versions.end()) {
        auto stream_reader_data_context{IrStreamReader::create_data_context(
                std::move(zstd_decompressor),
                std::move(data_buffer)
        )};

        return std::unique_ptr<IrStreamReader>(
                new IrStreamReader(std::move(stream_reader_data_context))
        );
    }
    SPDLOG_CRITICAL("Unable to create reader for CLP stream with version {}.", version);

    throw ClpFfiJsException{
            clp::ErrorCode::ErrorCode_Unsupported,
            __FILENAME__,
            __LINE__,
            std::format("Unable to create reader for CLP stream with version {}.", version)
    };
}
}  // namespace clp_ffi_js::ir

namespace {
EMSCRIPTEN_BINDINGS(ClpStreamReader) {
    // Output to JS types
    emscripten::register_type<clp_ffi_js::ir::DataArrayTsType>("Uint8Array");
    emscripten::register_type<clp_ffi_js::ir::DecodedResultsTsType>(
            "Array<[string, number, number, number]>"
    );
    emscripten::register_type<clp_ffi_js::ir::FilteredLogEventMapTsType>("number[] | null");
    // Input from JS types
    emscripten::register_type<clp_ffi_js::ir::LogLevelFilterTsType>("number[] | null");
    emscripten::class_<clp_ffi_js::ir::StreamReader>("ClpStreamReader")
            .constructor(
                    &clp_ffi_js::ir::StreamReader::create,
                    emscripten::return_value_policy::take_ownership()
            )
            .function(
                    "getNumEventsBuffered",
                    &clp_ffi_js::ir::StreamReader::get_num_events_buffered
            )
            .function(
                    "getFilteredLogEventMap",
                    &clp_ffi_js::ir::StreamReader::get_filtered_log_event_map
            )
            .function("filterLogEvents", &clp_ffi_js::ir::StreamReader::filter_log_events)
            .function("deserializeStream", &clp_ffi_js::ir::StreamReader::deserialize_stream)
            .function("decodeRange", &clp_ffi_js::ir::StreamReader::decode_range);
}
}  // namespace
