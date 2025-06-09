#include "decoding_methods.hpp"

#include <cstddef>
#include <cstdint>
#include <format>
#include <string>
#include <string_view>
#include <vector>

#include <clp/ErrorCode.hpp>
#include <clp/ffi/ir_stream/decoding_methods.hpp>
#include <clp/ReaderInterface.hpp>
#include <clp/TraceableException.hpp>
#include <clp/type_utils.hpp>
#include <emscripten/bind.h>
#include <json/single_include/nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <clp_ffi_js/ClpFfiJsException.hpp>
#include <clp_ffi_js/ir/StructuredIrStreamReader.hpp>
#include <clp_ffi_js/utils.hpp>

namespace clp_ffi_js::ir {
namespace {
using IRErrorCode = clp::ffi::ir_stream::IRErrorCode;
}  // namespace

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

auto deserialize_metadata(clp::ReaderInterface& reader) -> nlohmann::json {
    clp::ffi::ir_stream::encoded_tag_t metadata_type{};
    std::vector<int8_t> metadata_bytes;
    if (auto const err{
                clp::ffi::ir_stream::deserialize_preamble(reader, metadata_type, metadata_bytes)
        };
        IRErrorCode::IRErrorCode_Success != err)
    {
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

    try {
        // Deserialize metadata bytes as JSON.
        std::string_view const metadata_view{
                clp::size_checked_pointer_cast<char const>(metadata_bytes.data()),
                metadata_bytes.size()
        };
        return nlohmann::json::parse(metadata_view);
    } catch (nlohmann::json::exception const& e) {
        throw ClpFfiJsException{
                clp::ErrorCode::ErrorCode_MetadataCorrupted,
                __FILENAME__,
                __LINE__,
                std::format("Failed to parse stream's metadata: {}", e.what())
        };
    }
}

auto convert_metadata_to_js_object(nlohmann::json const& metadata) -> MetadataTsType {
    auto const metadata_str{dump_json_with_replace(metadata)};
    auto const metadata_obj{
            emscripten::val::global("JSON").call<emscripten::val>("parse", metadata_str)
    };
    return MetadataTsType{metadata_obj};
}
}  // namespace clp_ffi_js::ir
