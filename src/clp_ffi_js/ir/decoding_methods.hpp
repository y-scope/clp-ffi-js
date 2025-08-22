#ifndef CLP_FFI_JS_IR_DECODING_METHODS_HPP
#define CLP_FFI_JS_IR_DECODING_METHODS_HPP

#include <clp/ffi/ir_stream/Deserializer.hpp>
#include <clp/ffi/ir_stream/IrUnitHandlerReq.hpp>
#include <clp/ffi/ir_stream/search/QueryHandlerReq.hpp>
#include <clp/ReaderInterface.hpp>
#include <nlohmann/json.hpp>

#include <clp_ffi_js/ClpFfiJsException.hpp>
#include <clp_ffi_js/ir/StreamReader.hpp>
#include <clp_ffi_js/ir/StructuredIrStreamReader.hpp>

namespace clp_ffi_js::ir {
/**
 * Rewinds the reader to the beginning then validates the CLP IR data encoding type.
 *
 * @param reader
 * @throws ClpFfiJsException if the encoding type couldn't be decoded or the encoding type is
 * unsupported.
 */
auto rewind_reader_and_validate_encoding_type(clp::ReaderInterface& reader) -> void;

/**
 * Deserializes the metadata from the IR stream's preamble.
 *
 * @param reader
 * @throws ClpFfiJsException if the preamble couldn't be deserialized.
 * @return The IR stream's metadata as a JSON object.
 */
[[nodiscard]] auto deserialize_metadata(clp::ReaderInterface& reader) -> nlohmann::json;

/**
 * Converts the metadata from the given JSON object to a JavaScript object.
 *
 * @param metadata
 * @return The converted JavaScript object.
 */
[[nodiscard]] auto convert_metadata_to_js_object(nlohmann::json const& metadata) -> MetadataTsType;

template <
        clp::ffi::ir_stream::IrUnitHandlerReq IrUnitHandlerType,
        clp::ffi::ir_stream::search::QueryHandlerReq QueryHandlerType>
auto deserialize_log_events(
        clp::ffi::ir_stream::Deserializer<IrUnitHandlerType, QueryHandlerType>& deserializer,
        clp::ReaderInterface& reader
) -> void {
    while (false == deserializer.is_stream_completed()) {
        auto const result{deserializer.deserialize_next_ir_unit(reader)};
        if (false == result.has_error()) {
            continue;
        }
        auto const error{result.error()};
        if (std::errc::result_out_of_range == error) {
            SPDLOG_WARN("File contains an incomplete IR stream");
            break;
        }
        throw ClpFfiJsException{
                clp::ErrorCode::ErrorCode_Corrupt,
                __FILENAME__,
                __LINE__,
                std::format(
                        "Failed to deserialize IR unit: {}:{}",
                        error.category().name(),
                        error.message()
                )
        };
    }
}
}  // namespace clp_ffi_js::ir

#endif  // CLP_FFI_JS_IR_DECODING_METHODS_HPP
