#include "KVPairIRStreamReader.hpp"

#include <cstddef>
#include <cstdint>
#include <ffi/ir_stream/Deserializer.hpp>
#include <memory>
#include <span>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include <clp/Array.hpp>
#include <clp/ErrorCode.hpp>
#include <clp/ir/types.hpp>
#include <clp/streaming_compression/zstd/Decompressor.hpp>
#include <clp/TraceableException.hpp>
#include <emscripten/em_asm.h>
#include <emscripten/val.h>
#include <spdlog/spdlog.h>

#include <clp_ffi_js/ClpFfiJsException.hpp>
#include <clp_ffi_js/ir/StreamReader.hpp>
#include <clp_ffi_js/ir/StreamReaderDataContext.hpp>

using namespace std::literals::string_literals;
using clp::ir::four_byte_encoded_variable_t;

namespace clp_ffi_js::ir {
auto KVPairIRStreamReader::create(DataArrayTsType const& data_array) -> KVPairIRStreamReader {
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

    auto result{clp::ffi::ir_stream::Deserializer::create(*zstd_decompressor)};
    if (result.has_error()) {
        auto const error_code{result.error()};
        SPDLOG_CRITICAL(
                "Failed to create deserializer: {}:{}",
                error_code.category().name(),
                error_code.message()
        );
        throw ClpFfiJsException{
                clp::ErrorCode::ErrorCode_Failure,
                __FILENAME__,
                __LINE__,
                "Failed to create deserializer"
        };
    }

    StreamReaderDataContext stream_reader_data_context{
            std::move(data_buffer),
            std::move(zstd_decompressor),
            std::move(result.value())
    };
    return KVPairIRStreamReader{std::move(stream_reader_data_context)};
}

auto KVPairIRStreamReader::get_num_events_buffered() const -> size_t {
    return m_encoded_log_events.size();
}

auto KVPairIRStreamReader::get_filtered_log_event_map() const -> FilteredLogEventMapTsType {
    return FilteredLogEventMapTsType(emscripten::val::null());
}

auto KVPairIRStreamReader::filter_log_events(emscripten::val const& log_level_filter) -> void {}

auto KVPairIRStreamReader::deserialize_stream() -> size_t {
    if (nullptr != m_stream_reader_data_context) {
        constexpr size_t cDefaultNumReservedLogEvents{500'000};
        m_encoded_log_events.reserve(cDefaultNumReservedLogEvents);
        auto& reader{m_stream_reader_data_context->get_reader()};
        while (true) {
            auto result{m_stream_reader_data_context->get_deserializer()
                                .deserialize_to_next_log_event(reader)};
            if (false == result.has_error()) {
                m_encoded_log_events.emplace_back(std::move(result.value()));
                continue;
            }
            auto const error{result.error()};
            if (std::errc::no_message_available == error) {
                break;
            }
            if (std::errc::result_out_of_range == error) {
                SPDLOG_ERROR("File contains an incomplete IR stream");
                break;
            }
            throw ClpFfiJsException{
                    clp::ErrorCode::ErrorCode_Corrupt,
                    __FILENAME__,
                    __LINE__,
                    "Failed to deserialize: "s + error.category().name() + ":" + error.message()
            };
        }
        m_stream_reader_data_context.reset(nullptr);
    }

    return m_encoded_log_events.size();
}

auto KVPairIRStreamReader::decode_range(size_t begin_idx, size_t end_idx, bool /*use_filter*/) const
        -> DecodedResultsTsType {
    if (m_encoded_log_events.size() < end_idx || begin_idx >= end_idx) {
        return DecodedResultsTsType(emscripten::val::null());
    }

    std::span const log_events_span{
            m_encoded_log_events.begin()
                    + static_cast<decltype(m_encoded_log_events)::difference_type>(begin_idx),
            m_encoded_log_events.begin()
                    + static_cast<decltype(m_encoded_log_events)::difference_type>(end_idx)
    };
    size_t log_num{begin_idx + 1};
    auto const results{emscripten::val::array()};
    for (auto const& log_event : log_events_span) {
        auto const json{log_event.serialize_to_json()};
        if (false == json.has_value()) {
            SPDLOG_ERROR("Failed to decode message.");
            break;
        }

        EM_ASM(
                { Emval.toValue($0).push([UTF8ToString($1), $2]); },
                results.as_handle(),
                json.value().dump().c_str(),
                log_num
        );
        ++log_num;
    }

    return DecodedResultsTsType(results);
}

KVPairIRStreamReader::KVPairIRStreamReader(
        StreamReaderDataContext<deserializer_t>&& stream_reader_data_context
)
        : m_stream_reader_data_context{std::make_unique<StreamReaderDataContext<deserializer_t>>(
                  std::move(stream_reader_data_context)
          )} {}
}  // namespace clp_ffi_js::ir

namespace {
EMSCRIPTEN_BINDINGS(ClpIrStreamReader) {
    emscripten::register_type<clp_ffi_js::ir::DataArrayTsType>("Uint8Array");
    emscripten::register_type<clp_ffi_js::ir::DecodedResultsTsType>("Array<[string, number]>");
    emscripten::class_<
            clp_ffi_js::ir::KVPairIRStreamReader,
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
