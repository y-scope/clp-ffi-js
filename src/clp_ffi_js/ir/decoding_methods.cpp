#include "decoding_methods.hpp"

#include <cstdint>
#include <format>
#include <json/single_include/nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <vector>

#include <clp/ErrorCode.hpp>
#include <clp/ffi/ir_stream/decoding_methods.hpp>
#include <clp/ffi/ir_stream/protocol_constants.hpp>
#include <clp/ReaderInterface.hpp>
#include <clp/TraceableException.hpp>
#include <clp/type_utils.hpp>
#include <spdlog/spdlog.h>

#include <clp_ffi_js/ClpFfiJsException.hpp>

namespace clp_ffi_js::ir {
auto rewind_reader_and_validate_encoding_type(clp::ReaderInterface& reader) -> void {
    reader.seek_from_begin(0);

    bool is_four_bytes_encoding{true};
    if (auto const err{clp::ffi::ir_stream::get_encoding_type(reader, is_four_bytes_encoding)};
        clp::ffi::ir_stream::IRErrorCode::IRErrorCode_Success != err)
    {
        SPDLOG_CRITICAL("Failed to decode encoding type, err={}", err);
        throw ClpFfiJsException{
                clp::ErrorCode::ErrorCode_MetadataCorrupted,
                __FILENAME__,
                __LINE__,
                "Failed to decode encoding type."
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

auto get_version(clp::ReaderInterface& reader) -> std::string {
    // The encoding type bytes must be consumed before the metadata can be read.
    rewind_reader_and_validate_encoding_type(reader);

    // Deserialize metadata bytes from preamble.
    clp::ffi::ir_stream::encoded_tag_t metadata_type{};
    std::vector<int8_t> metadata_bytes;
    auto const deserialize_preamble_result{
            clp::ffi::ir_stream::deserialize_preamble(reader, metadata_type, metadata_bytes)
    };
    if (clp::ffi::ir_stream::IRErrorCode::IRErrorCode_Success != deserialize_preamble_result) {
        SPDLOG_CRITICAL("Failed to deserialize preamble for version reading");

        throw ClpFfiJsException{
                clp::ErrorCode::ErrorCode_Failure,
                __FILENAME__,
                __LINE__,
                "Failed to deserialize preamble for version reading"
        };
    }

    // Deserialize metadata bytes which is encoded in JSON.
    std::string_view const metadata_view{
            clp::size_checked_pointer_cast<char const>(metadata_bytes.data()),
            metadata_bytes.size()
    };

    std::string version;
    try {
        nlohmann::json const metadata = nlohmann::json::parse(metadata_view);
        version = metadata.at(clp::ffi::ir_stream::cProtocol::Metadata::VersionKey);
    } catch (nlohmann::json::exception const& e) {
        throw ClpFfiJsException{
                clp::ErrorCode::ErrorCode_MetadataCorrupted,
                __FILENAME__,
                __LINE__,
                std::format("Error parsing stream metadata: {}", e.what())
        };
    }

    SPDLOG_INFO("The version is {}", version);
    return version;
}
}  // namespace clp_ffi_js::ir
