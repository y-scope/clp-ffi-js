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
auto KVPairIRStreamReader::create(DataArrayTsType const& data_array, ReaderOptions const& reader_options) -> KVPairIRStreamReader {
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

    // FIXME: Note we create a vector to store log events in IrUnitHandler at the moment. We should create a
    // std::vector<LogEventClass> to be shared by IrUnitHandler() and KVPairIRStreamReader
    // so that even after we free the m_stream_reader_data_context in the KVPairIRStreamReader instance (after we
    // deserialize and stored all events in deserialize_stream), we can still access the stored log events.
    auto result{clp::ffi::ir_stream::Deserializer<IrUnitHandler>::create(*zstd_decompressor, IrUnitHandler())};
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
    return KVPairIRStreamReader{std::move(stream_reader_data_context), std::move(reader_options)};
}

auto KVPairIRStreamReader::get_num_events_buffered() const -> size_t {
    // FIXME: we should not access the vector in ir_unit_handler. See above for reasons.
    return m_stream_reader_data_context->get_deserializer().get_ir_unit_handler().get_deserialized_log_events().size();
}

auto KVPairIRStreamReader::get_filtered_log_event_map() const -> FilteredLogEventMapTsType {
    if (false == m_filtered_log_event_map.has_value()) {
        return FilteredLogEventMapTsType{emscripten::val::null()};
    }

    return FilteredLogEventMapTsType{emscripten::val::array(m_filtered_log_event_map.value())};
}

auto KVPairIRStreamReader::filter_log_events(emscripten::val const& log_level_filter) -> void {
    if (log_level_filter.isNull()) {
        m_filtered_log_event_map.reset();
        return;
    }

    m_filtered_log_event_map.emplace();
    auto filter_levels{emscripten::vecFromJSArray<std::underlying_type_t<LogLevel>>(log_level_filter
    )};
    for (size_t log_event_idx = 0; log_event_idx < m_encoded_log_events.size(); ++log_event_idx) {
        auto const& log_event = m_encoded_log_events[log_event_idx];
        if (std::ranges::find(
                filter_levels,
                clp::enum_to_underlying_type(log_event.get_log_level())
        )
            != filter_levels.end())
        {
            m_filtered_log_event_map->emplace_back(log_event_idx);
        }
    }
}

auto KVPairIRStreamReader::deserialize_stream() -> size_t {
    if (nullptr != m_stream_reader_data_context) {
        constexpr size_t cDefaultNumReservedLogEvents{500'000};
        m_encoded_log_events.reserve(cDefaultNumReservedLogEvents);
        auto& reader{m_stream_reader_data_context->get_reader()};
        clp::ffi::SchemaTree::Node::id_t log_level_node_id{clp::ffi::SchemaTree::cRootId};
        while (true) {
            auto result{m_stream_reader_data_context->get_deserializer().deserialize_next_ir_unit(reader)};
            if (false == result.has_error()) {
                // FIXME: revisit: we don't really need to parse log levels from key-value-pair log events since the
                // levels are assumed to be stored directly as some node as a integer (which depends on our loglib
                // implementations) and can be directly access via a given node_id.
//                auto &log_events{m_stream_reader_data_context->get_deserializer().get_ir_unit_handler().get_deserialized_log_events()};
//                auto &last_event{log_events[log_events.size() - 1]};
//                LogLevel log_level{LogLevel::NONE};
//                if (clp::ffi::SchemaTree::cRootId == log_level_node_id) {
//                    auto const log_level_node_id_result{last_event.get_schema_tree().try_get_node_id(
//                            {clp::ffi::SchemaTree::cRootId, m_log_level_key, clp::ffi::SchemaTree::Node::Type::Str}
//                    )};
//                    if (log_level_node_id_result.has_value()) {
//                        log_level_node_id = log_level_node_id_result.value();
//                    }
//                } else {
//                    auto const &log_level_value{last_event.get_node_id_value_pairs().at(log_level_node_id)};
//                    if (log_level_value.has_value() &&
//                        "short_string" == log_level_value.value().get_immutable_view<std::string>()) {
//                        log_level = LogLevel::ERROR;
//                    }
//                }
//
//                auto log_event_with_level {LogEventWithLevel<clp::ffi::KeyValuePairLogEvent>(
//                        std::move(last_event),
//                       log_level
//                        )};
//                m_encoded_log_events.emplace_back(std::move(log_event_with_level));
                continue;
            }
            auto const error{result.error()};
            if (std::errc::no_message_available == error || std::errc::operation_not_permitted == error) {
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
        // FIXME: we should really free this after we decode all events. See the FIXME in ::create() about why we
        // commented out below line.
//        m_stream_reader_data_context.reset(nullptr);
    }

    return m_stream_reader_data_context->get_deserializer().get_ir_unit_handler().get_deserialized_log_events().size();
}

auto KVPairIRStreamReader::decode_range(size_t begin_idx, size_t end_idx, bool /*use_filter*/) const
        -> DecodedResultsTsType {
    // FIXME: we should not access the vector from the ir_unit_handler. See above for reasons.
    if (m_stream_reader_data_context->get_deserializer().get_ir_unit_handler().get_deserialized_log_events().size() < end_idx || begin_idx >= end_idx) {
        return DecodedResultsTsType(emscripten::val::null());
    }

    std::span const log_events_span{
            m_stream_reader_data_context->get_deserializer().get_ir_unit_handler().get_deserialized_log_events().begin()
                    + static_cast<std::vector<clp::ffi::KeyValuePairLogEvent>::difference_type>(begin_idx),
            m_stream_reader_data_context->get_deserializer().get_ir_unit_handler().get_deserialized_log_events().begin()
                    + static_cast<std::vector<clp::ffi::KeyValuePairLogEvent>::difference_type>(end_idx)
    };
    size_t log_num{begin_idx + 1};
    auto const results{emscripten::val::array()};
//    for (auto const& log_event_with_level : log_events_span) {
//        auto const json{log_event_with_level.get_log_event().serialize_to_json()};
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
        StreamReaderDataContext<deserializer_t>&& stream_reader_data_context, ReaderOptions const& reader_options
)
        : m_stream_reader_data_context{std::make_unique<StreamReaderDataContext<deserializer_t>>(
                  std::move(stream_reader_data_context)
          )},
          m_log_level_key{reader_options["logLevelKey"].as<std::string>()},
          m_timestamp_key{reader_options["timestampKey"].as<std::string>()} {}
}  // namespace clp_ffi_js::ir

