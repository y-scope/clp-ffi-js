#include "StreamReader.hpp"

#include <cstddef>
#include <cstdint>
#include <ErrorCode.hpp>
#include <format>
#include <memory>
#include <utility>

#include <clp/Array.hpp>
#include <clp/streaming_compression/zstd/Decompressor.hpp>
#include <clp/TraceableException.hpp>
#include <emscripten/bind.h>
#include <spdlog/spdlog.h>

#include <clp_ffi_js/ClpFfiJsException.hpp>
#include <clp_ffi_js/ir/decoding_methods.hpp>
#include <clp_ffi_js/ir/IRStreamReader.hpp>

namespace clp_ffi_js::ir {
auto StreamReader::create(DataArrayTsType const& data_array) -> std::unique_ptr<StreamReader> {
    auto const length{data_array["length"].as<size_t>()};
    SPDLOG_INFO("KVPairIRStreamReader::create: got buffer of length={}", length);

    // Copy array from JavaScript to C++
    clp::Array<char> data_buffer{length};
    // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
    emscripten::val::module_property("HEAPU8")
            .call<void>("set", data_array, reinterpret_cast<uintptr_t>(data_buffer.data()));
    // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)

    auto zstd_decompressor{std::make_unique<clp::streaming_compression::zstd::Decompressor>()};
    zstd_decompressor->open(data_buffer.data(), length);

    auto const version{get_version(*zstd_decompressor)};
    if (version == "v0.0.0") {
        auto stream_reader_data_context{IRStreamReader::create_deserializer_and_data_context(
                std::move(zstd_decompressor),
                std::move(data_buffer)
        )};

        return std::unique_ptr<IRStreamReader>(
                new IRStreamReader(std::move(stream_reader_data_context))
        );
    }

    throw ClpFfiJsException{
            clp::ErrorCode::ErrorCode_Unsupported,
            __FILENAME__,
            __LINE__,
            std::format("Unable to create stream reader for IR data with version {}.", version)
    };
}
}  // namespace clp_ffi_js::ir

namespace {
EMSCRIPTEN_BINDINGS(ClpStreamReader) {
    emscripten::register_type<clp_ffi_js::ir::DataArrayTsType>("Uint8Array");
    emscripten::register_type<clp_ffi_js::ir::DecodedResultsTsType>(
            "Array<[string, number, number, number]>"
    );
    emscripten::register_type<clp_ffi_js::ir::FilteredLogEventMapTsType>("number[] | null");

    emscripten::class_<clp_ffi_js::ir::StreamReader>("ClpStreamReader")
            .constructor(
                    &clp_ffi_js::ir::StreamReader::create,
                    emscripten::return_value_policy::take_ownership()
            );
}
}  // namespace
