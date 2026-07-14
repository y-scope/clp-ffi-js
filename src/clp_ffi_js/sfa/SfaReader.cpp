#include "SfaReader.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include <clp_s/ffi/sfa/ClpArchiveReader.hpp>
#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include <clp_ffi_js/binding_types.hpp>

namespace clp_ffi_js::sfa {
using clp_ffi_js::DataArrayTsType;
using clp_ffi_js::StringArrayTsType;

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
        auto entry{emscripten::val::object()};
        entry.set("fileName", emscripten::val(file_info.get_file_name()));
        entry.set("logEventIdxStart", emscripten::val(file_info.get_start_index()));
        entry.set("logEventIdxEnd", emscripten::val(file_info.get_end_index()));
        entry.set("logEventCount", emscripten::val(file_info.get_event_count()));
        file_infos.call<void>("push", entry);
    }
    return FileInfoArrayTsType{file_infos};
}

auto SfaReader::decode_all() -> LogEventArrayTsType {
    auto decoded_result{m_reader.decode_all()};
    if (decoded_result.has_error()) {
        auto const error{decoded_result.error()};
        auto const err_msg{fmt::format(
                "Failed to decode SFA archive: {} - {}.",
                error.category().name(),
                error.message()
        )};
        SPDLOG_ERROR("{}", err_msg);
        throw std::runtime_error{err_msg};
    }

    auto decoded_events{emscripten::val::array()};
    for (auto const& event : decoded_result.value()) {
        auto entry{emscripten::val::object()};
        entry.set("logEventIdx", emscripten::val(event.get_log_event_idx()));
        entry.set("timestamp", emscripten::val(event.get_timestamp()));
        entry.set("message", emscripten::val(event.get_message()));
        decoded_events.call<void>("push", entry);
    }
    return LogEventArrayTsType{decoded_events};
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

    emscripten::class_<clp_ffi_js::sfa::SfaReader>("ClpSfaReader")
            .constructor(
                    &clp_ffi_js::sfa::SfaReader::create,
                    emscripten::return_value_policy::take_ownership()
            )
            .function("getEventCount", &clp_ffi_js::sfa::SfaReader::get_event_count)
            .function("getFileNames", &clp_ffi_js::sfa::SfaReader::get_file_names)
            .function("getFileInfos", &clp_ffi_js::sfa::SfaReader::get_file_infos)
            .function("decodeAll", &clp_ffi_js::sfa::SfaReader::decode_all);
}
