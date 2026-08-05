#include "SfaReader.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <clp_s/ffi/sfa/ClpArchiveReader.hpp>
#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include <clp_ffi_js/binding_types.hpp>
#include <clp_ffi_js/utils.hpp>

namespace clp_ffi_js::sfa {
using clp_ffi_js::DataArrayTsType;
using clp_ffi_js::StringArrayTsType;

namespace {
template <typename ValueType>
auto
throw_if_error(ystdlib::error_handling::Result<ValueType> const& result, std::string_view operation)
        -> void {
    if (result.has_error()) {
        auto const error{result.error()};
        auto const err_msg{fmt::format(
                "Failed to {} SFA archive: {} - {}.",
                operation,
                error.category().name(),
                error.message()
        )};
        SPDLOG_ERROR("{}", err_msg);
        throw std::runtime_error{err_msg};
    }
}

auto create_file_info(clp_s::ffi::sfa::FileInfo const& file_info) -> emscripten::val {
    auto entry{emscripten::val::object()};
    entry.set("fileName", emscripten::val(file_info.get_file_name()));
    entry.set("logEventIdxStart", emscripten::val(file_info.get_start_index()));
    entry.set("logEventIdxEnd", emscripten::val(file_info.get_end_index()));
    entry.set("logEventCount", emscripten::val(file_info.get_event_count()));
    return entry;
}

auto create_log_event_array(clp_s::ffi::sfa::LogEventView events, int64_t log_event_idx_offset)
        -> emscripten::val {
    auto decoded_events{emscripten::val::array()};
    for (auto const& event : events) {
        auto entry{emscripten::val::object()};
        entry.set(
                "logEventIdx",
                emscripten::val(event.get_log_event_idx() - log_event_idx_offset)
        );
        entry.set("timestamp", emscripten::val(event.get_timestamp()));
        entry.set("message", emscripten::val(event.get_message()));
        decoded_events.call<void>("push", entry);
    }
    return decoded_events;
}
}  // namespace

auto SfaReader::create(DataArrayTsType const& data_array) -> std::unique_ptr<SfaReader> {
    auto const length{data_array["length"].as<size_t>()};
    SPDLOG_INFO("SfaReader::create: got buffer of length={}", length);

    // Copy array from JavaScript to C++.
    std::vector<char> data_buffer(length);
    // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
    emscripten::val::module_property("HEAPU8")
            .call<void>("set", data_array, reinterpret_cast<uintptr_t>(data_buffer.data()));
    // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)

    auto reader_result{clp_s::ffi::sfa::ClpArchiveReader::create(std::move(data_buffer))};

    if (reader_result.has_error()) {
        auto const error{reader_result.error()};
        auto const err_msg{fmt::format(
                "Failed to open SFA archive from buffer: {} - {}.",
                error.category().name(),
                error.message()
        )};
        SPDLOG_ERROR("{}", err_msg);
        throw std::runtime_error{err_msg};
    }

    return std::unique_ptr<SfaReader>{new SfaReader{std::move(reader_result.value())}};
}

auto SfaReader::get_file_names() const -> StringArrayTsType {
    auto file_names{emscripten::val::array()};
    for (auto const& file_name : m_reader.get_file_names()) {
        file_names.call<void>("push", emscripten::val(file_name));
    }
    return StringArrayTsType{file_names};
}

auto SfaReader::get_file_infos() const -> FileInfoArrayTsType {
    auto file_infos{emscripten::val::array()};
    for (auto const& file_info : m_reader.get_file_infos()) {
        file_infos.call<void>("push", create_file_info(file_info));
    }
    return FileInfoArrayTsType{file_infos};
}

auto SfaReader::get_file_info(std::string const& file_name) const -> NullableFileInfoTsType {
    auto const file_info{m_reader.find_file_info(file_name)};
    if (false == file_info.has_value()) {
        return NullableFileInfoTsType{emscripten::val::null()};
    }
    return NullableFileInfoTsType{create_file_info(file_info.value())};
}

auto SfaReader::get_selected_file_name() const -> NullableStringTsType {
    auto const file_info{m_reader.get_selected_file_info()};
    if (false == file_info.has_value()) {
        return NullableStringTsType{emscripten::val::null()};
    }
    return NullableStringTsType{emscripten::val(file_info->get_file_name())};
}

void SfaReader::select_file(std::string const& file_name) {
    auto select_result{m_reader.select_file(file_name)};
    throw_if_error(select_result, "select source file from");
    m_filtered_log_event_map.reset();
}

auto SfaReader::get_filtered_log_event_map() const -> FilteredLogEventMapTsType {
    if (false == m_filtered_log_event_map.has_value()) {
        return FilteredLogEventMapTsType{emscripten::val::null()};
    }
    return FilteredLogEventMapTsType{emscripten::val::array(*m_filtered_log_event_map)};
}

void SfaReader::filter_log_events(
        std::string const& kql_filter,
        std::string const& log_level_kql_filter
) {
    m_filtered_log_event_map.reset();
    FilteredLogEventsMap filtered_log_event_map;

    if (false == kql_filter.empty()) {
        auto search_result{m_reader.search(kql_filter, false)};
        throw_if_error(search_result, "search");
        filtered_log_event_map.emplace(std::move(search_result.value()));
    }

    if (false == log_level_kql_filter.empty()) {
        auto search_result{m_reader.search(log_level_kql_filter, true)};
        throw_if_error(search_result, "search");
        if (filtered_log_event_map.has_value()) {
            std::vector<size_t> intersection;
            intersection.reserve(
                    std::min(filtered_log_event_map->size(), search_result.value().size())
            );
            std::set_intersection(
                    filtered_log_event_map->begin(),
                    filtered_log_event_map->end(),
                    search_result.value().begin(),
                    search_result.value().end(),
                    std::back_inserter(intersection)
            );
            filtered_log_event_map = std::move(intersection);
        } else {
            filtered_log_event_map.emplace(std::move(search_result.value()));
        }
    }

    if (filtered_log_event_map.has_value()
        && filtered_log_event_map->size() == m_reader.get_active_event_count())
    {
        filtered_log_event_map.reset();
    }
    m_filtered_log_event_map = std::move(filtered_log_event_map);
}

auto SfaReader::decode() -> void {
    auto decoded_result{m_reader.decode()};
    throw_if_error(decoded_result, "decode");
}

auto SfaReader::decode_all() -> LogEventArrayTsType {
    auto decoded_result{m_reader.decode_all()};
    throw_if_error(decoded_result, "decode");
    return LogEventArrayTsType{
            create_log_event_array(decoded_result.value(), get_log_event_idx_offset())
    };
}

auto SfaReader::decode_range(size_t begin_idx, size_t end_idx, bool use_filter)
        -> NullableLogEventArrayTsType {
    if (use_filter && false == m_filtered_log_event_map.has_value()) {
        return NullableLogEventArrayTsType{emscripten::val::null()};
    }

    auto const collection_size{
            use_filter ? m_filtered_log_event_map->size()
                       : static_cast<size_t>(m_reader.get_active_event_count())
    };
    if (begin_idx > end_idx || end_idx > collection_size) {
        return NullableLogEventArrayTsType{emscripten::val::null()};
    }

    if (false == use_filter) {
        auto decoded_result{m_reader.decode_range(begin_idx, end_idx)};
        throw_if_error(decoded_result, "decode");
        return NullableLogEventArrayTsType{
                create_log_event_array(decoded_result.value(), get_log_event_idx_offset())
        };
    }

    auto decoded_result{m_reader.decode_all()};
    throw_if_error(decoded_result, "decode");
    auto decoded_events{emscripten::val::array()};
    auto const log_event_idx_offset{get_log_event_idx_offset()};
    for (size_t filtered_idx{begin_idx}; filtered_idx < end_idx; ++filtered_idx) {
        auto const log_event_idx{m_filtered_log_event_map->at(filtered_idx)};
        auto const& event{decoded_result.value()[log_event_idx]};
        auto entry{emscripten::val::object()};
        entry.set(
                "logEventIdx",
                emscripten::val(event.get_log_event_idx() - log_event_idx_offset)
        );
        entry.set("timestamp", emscripten::val(event.get_timestamp()));
        entry.set("message", emscripten::val(event.get_message()));
        decoded_events.call<void>("push", entry);
    }
    return NullableLogEventArrayTsType{decoded_events};
}

auto SfaReader::find_nearest_log_event_by_timestamp(int64_t target_timestamp)
        -> clp_ffi_js::NullableLogEventIdx {
    auto decoded_result{m_reader.decode_all()};
    throw_if_error(decoded_result, "decode");

    auto const optional_log_event_idx{clp_ffi_js::find_nearest_log_event_by_timestamp(
            decoded_result.value(),
            target_timestamp
    )};
    if (false == optional_log_event_idx.has_value()) {
        return clp_ffi_js::NullableLogEventIdx{emscripten::val::null()};
    }
    return clp_ffi_js::NullableLogEventIdx{emscripten::val{optional_log_event_idx.value()}};
}

auto SfaReader::get_log_event_idx_offset() const -> int64_t {
    auto const file_info{m_reader.get_selected_file_info()};
    return file_info.has_value() ? file_info->get_start_index() : 0;
}
}  // namespace clp_ffi_js::sfa

EMSCRIPTEN_BINDINGS(SfaReader) {
    emscripten::register_type<clp_ffi_js::sfa::FileInfoArrayTsType>(
            "Array<{fileName: string, logEventIdxStart: bigint, logEventIdxEnd: bigint, "
            "logEventCount: bigint}>"
    );
    emscripten::register_type<clp_ffi_js::sfa::LogEventArrayTsType>(
            "Array<{logEventIdx: bigint, timestamp: bigint, message: string}>"
    );
    emscripten::register_type<clp_ffi_js::sfa::NullableFileInfoTsType>(
            "{fileName: string, logEventIdxStart: bigint, logEventIdxEnd: bigint, "
            "logEventCount: bigint} | null"
    );
    emscripten::register_type<clp_ffi_js::sfa::NullableLogEventArrayTsType>(
            "Array<{logEventIdx: bigint, timestamp: bigint, message: string}> | null"
    );
    emscripten::register_type<clp_ffi_js::sfa::FilteredLogEventMapTsType>("number[] | null");
    emscripten::register_type<clp_ffi_js::sfa::NullableStringTsType>("string | null");

    emscripten::class_<clp_ffi_js::sfa::SfaReader>("ClpSfaReader")
            .constructor(
                    &clp_ffi_js::sfa::SfaReader::create,
                    emscripten::return_value_policy::take_ownership()
            )
            .function("getEventCount", &clp_ffi_js::sfa::SfaReader::get_event_count)
            .function("getActiveEventCount", &clp_ffi_js::sfa::SfaReader::get_active_event_count)
            .function("getUncompressedSize", &clp_ffi_js::sfa::SfaReader::get_uncompressed_size)
            .function("getFileNames", &clp_ffi_js::sfa::SfaReader::get_file_names)
            .function("getFileInfos", &clp_ffi_js::sfa::SfaReader::get_file_infos)
            .function("getFileInfo", &clp_ffi_js::sfa::SfaReader::get_file_info)
            .function(
                    "getSelectedFileName",
                    &clp_ffi_js::sfa::SfaReader::get_selected_file_name
            )
            .function("selectFile", &clp_ffi_js::sfa::SfaReader::select_file)
            .function(
                    "getFilteredLogEventMap",
                    &clp_ffi_js::sfa::SfaReader::get_filtered_log_event_map
            )
            .function("filterLogEvents", &clp_ffi_js::sfa::SfaReader::filter_log_events)
            .function("clearQuery", &clp_ffi_js::sfa::SfaReader::clear_query)
            .function("decode", &clp_ffi_js::sfa::SfaReader::decode)
            .function("decodeAll", &clp_ffi_js::sfa::SfaReader::decode_all)
            .function("decodeRange", &clp_ffi_js::sfa::SfaReader::decode_range)
            .function(
                    "findNearestLogEventByTimestamp",
                    &clp_ffi_js::sfa::SfaReader::find_nearest_log_event_by_timestamp
            );
}
