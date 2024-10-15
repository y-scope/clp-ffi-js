#include "decoding_methods.hpp"

#include <clp/ErrorCode.hpp>
#include <clp/ffi/ir_stream/decoding_methods.hpp>
#include <clp/ReaderInterface.hpp>
#include <clp/TraceableException.hpp>
#include <spdlog/spdlog.h>

#include <clp_ffi_js/ClpFfiJsException.hpp>

namespace clp_ffi_js::ir {
auto rewind_reader_and_verify_encoding_type(clp::ReaderInterface& reader) -> void {
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
}  // namespace clp_ffi_js::ir
