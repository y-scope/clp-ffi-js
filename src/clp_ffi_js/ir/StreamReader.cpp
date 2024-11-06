#include "StreamReader.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <format>
#include <json/single_include/nlohmann/json.hpp>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <clp/Array.hpp>
#include <clp/ErrorCode.hpp>
#include <clp/ffi/ir_stream/decoding_methods.hpp>
#include <clp/ffi/ir_stream/protocol_constants.hpp>
#include <clp/ReaderInterface.hpp>
#include <clp/streaming_compression/zstd/Decompressor.hpp>
#include <clp/TraceableException.hpp>
#include <clp/type_utils.hpp>
#include <emscripten/bind.h>
#include <spdlog/spdlog.h>

#include <clp_ffi_js/ClpFfiJsException.hpp>
#include <clp_ffi_js/ir/UnstructuredIrStreamReader.hpp>

namespace {
using ClpFfiJsException = clp_ffi_js::ClpFfiJsException;
using IRErrorCode = clp::ffi::ir_stream::IRErrorCode;

/**
 * Rewinds the reader to the beginning then validates the CLP IR data encoding type.
 * @param reader
 * @throws ClpFfiJsException if the encoding type couldn't be decoded or the encoding type is
 * unsupported.
 */
auto rewind_reader_and_validate_encoding_type(clp::ReaderInterface& reader) -> void {
    reader.seek_from_begin(0);

    bool is_four_bytes_encoding{true};
    if (auto const err{clp::ffi::ir_stream::get_encoding_type(reader, is_four_bytes_encoding)};
        IRErrorCode::IRErrorCode_Success != err)
    {
        throw ClpFfiJsException{
                clp::ErrorCode::ErrorCode_MetadataCorrupted,
                __FILENAME__,
                __LINE__,
                std::format(
                        "Failed to decode encoding type: IR error code {}",
                        clp::enum_to_underlying_type(err)
                )
        };
    }
    if (false == is_four_bytes_encoding) {
        throw ClpFfiJsException{
                clp::ErrorCode::ErrorCode_Unsupported,
                __FILENAME__,
                __LINE__,
                "IR stream uses unsupported encoding."
        };
    }
}

/**
 * Gets the version of the IR stream.
 * @param reader
 * @throws ClpFfiJsException if the preamble couldn't be deserialized.
 * @return The IR stream's version.
 */
auto get_version(clp::ReaderInterface& reader) -> std::string {
    // Deserialize metadata bytes from preamble.
    clp::ffi::ir_stream::encoded_tag_t metadata_type{};
    std::vector<int8_t> metadata_bytes;
    auto const err{clp::ffi::ir_stream::deserialize_preamble(reader, metadata_type, metadata_bytes)
    };
    if (IRErrorCode::IRErrorCode_Success != err) {
        throw ClpFfiJsException{
                clp::ErrorCode::ErrorCode_Failure,
                __FILENAME__,
                __LINE__,
                std::format(
                        "Failed to deserialize preamble: IR error code {}",
                        clp::enum_to_underlying_type(err)
                )
        };
    }

    std::string version;
    try {
        // Deserialize metadata bytes as JSON.
        std::string_view const metadata_view{
                clp::size_checked_pointer_cast<char const>(metadata_bytes.data()),
                metadata_bytes.size()
        };
        nlohmann::json const metadata = nlohmann::json::parse(metadata_view);
        version = metadata.at(clp::ffi::ir_stream::cProtocol::Metadata::VersionKey);
    } catch (nlohmann::json::exception const& e) {
        throw ClpFfiJsException{
                clp::ErrorCode::ErrorCode_MetadataCorrupted,
                __FILENAME__,
                __LINE__,
                std::format("Failed to parse stream's metadata: {}", e.what())
        };
    }

    SPDLOG_INFO("IR version is {}", version);
    return version;
}

EMSCRIPTEN_BINDINGS(ClpStreamReader) {
    // JS types used as inputs
    emscripten::register_type<clp_ffi_js::ir::DataArrayTsType>("Uint8Array");
    emscripten::register_type<clp_ffi_js::ir::LogLevelFilterTsType>("number[] | null");

    // JS types used as outputs
    emscripten::register_type<clp_ffi_js::ir::DecodedResultsTsType>(
            "Array<[string, number, number, number]>"
    );
    emscripten::register_type<clp_ffi_js::ir::FilteredLogEventMapTsType>("number[] | null");
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

    auto zstd_decompressor{std::make_unique<ZstdDecompressor>()};
    zstd_decompressor->open(data_buffer.data(), length);

    // Required to validate encoding type prior to getting version.
    rewind_reader_and_validate_encoding_type(*zstd_decompressor);
    auto const version{get_version(*zstd_decompressor)};

    // Required that reader offset matches position after validation in order to decode log events.
    rewind_reader_and_validate_encoding_type(*zstd_decompressor);
    if (std::ranges::find(cUnstructuredIrVersions, version) != cUnstructuredIrVersions.end()) {
        return std::make_unique<UnstructuredIrStreamReader>(UnstructuredIrStreamReader::create(
                std::move(zstd_decompressor),
                std::move(data_buffer)
        ));
    }
    SPDLOG_INFO("did i get here 3");

    throw ClpFfiJsException{
            clp::ErrorCode::ErrorCode_Unsupported,
            __FILENAME__,
            __LINE__,
            std::format("Unable to create reader for IR stream with version {}.", version)
    };
}
}  // namespace clp_ffi_js::ir
