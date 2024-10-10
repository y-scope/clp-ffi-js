#include "StreamReader.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include <clp/Array.hpp>
#include <clp/ErrorCode.hpp>
#include <clp/ffi/ir_stream/decoding_methods.hpp>
#include <clp/ffi/ir_stream/Deserializer.hpp>
#include <clp/ir/LogEventDeserializer.hpp>
#include <clp/ir/types.hpp>
#include <clp/streaming_compression/zstd/Decompressor.hpp>
#include <clp/TraceableException.hpp>
#include <emscripten/em_asm.h>
#include <emscripten/val.h>
#include <spdlog/spdlog.h>

#include <clp_ffi_js/ClpFfiJsException.hpp>
#include <clp_ffi_js/constants.hpp>
#include <clp_ffi_js/ir/StreamReaderDataContext.hpp>

#include "KVPairIRStreamReader.hpp"

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

    bool is_four_byte_encoding{};
    auto const get_encoding_type_result{
            clp::ffi::ir_stream::get_encoding_type(*zstd_decompressor, is_four_byte_encoding)
    };
    if (clp::ffi::ir_stream::IRErrorCode::IRErrorCode_Success != get_encoding_type_result) {
        SPDLOG_CRITICAL("Failed to get encoding type: {}", get_encoding_type_result);
        throw ClpFfiJsException{
                clp::ErrorCode::ErrorCode_Failure,
                __FILENAME__,
                __LINE__,
                "Failed to get encoding type."
        };
    }
    clp::ffi::ir_stream::encoded_tag_t metadata_type{};
    std::vector<int8_t> metadata_bytes;
    auto const deserialize_preamble_result{clp::ffi::ir_stream::deserialize_preamble(
            *zstd_decompressor,
            metadata_type,
            metadata_bytes
    )};
    if (clp::ffi::ir_stream::IRErrorCode::IRErrorCode_Success != deserialize_preamble_result) {
        SPDLOG_CRITICAL(
                "Failed to deserialize preamble for version reading: {}",
                deserialize_preamble_result
        );
        throw ClpFfiJsException{
                clp::ErrorCode::ErrorCode_Failure,
                __FILENAME__,
                __LINE__,
                "Failed to deserialize preamble for version reading."
        };
    }
    std::string_view const metadata_view{
            clp::size_checked_pointer_cast<char const>(metadata_bytes.data()),
            metadata_bytes.size()
    };
    nlohmann::json const metadata = nlohmann::json::parse(metadata_view);
    auto const& version{metadata.at(clp::ffi::ir_stream::cProtocol::Metadata::VersionKey)};
    if (version == "v0.0.0") {
        SPDLOG_CRITICAL("this is irv1; gg");
        throw ClpFfiJsException{
                clp::ErrorCode::ErrorCode_Failure,
                __FILENAME__,
                __LINE__,
                "this is irv1; gg."
        };
    }
    SPDLOG_INFO("The version is {}", version);

    return std::make_unique<KVPairIRStreamReader>(KVPairIRStreamReader::create(std::move(data_array)
    ));
}
}  // namespace clp_ffi_js::ir
