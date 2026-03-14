#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <clp/ErrorCode.hpp>
#include <clp_s/ffi/sfa/ClpArchiveReader.hpp>
#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <fmt/core.h>
#include <spdlog/spdlog.h>

#include <clp_ffi_js/ir/StreamReader.hpp>
#include <clp_ffi_js/ClpFfiJsException.hpp>

namespace clp_ffi_js::sfa {
class SfaReader {
public:
    static auto create(ir::DataArrayTsType const& data_array) -> std::unique_ptr<SfaReader> {
        auto const length{data_array["length"].as<size_t>()};
        SPDLOG_INFO("SfaReader::create: got buffer of length={}", length);

        // Copy array from JavaScript to C++.
        std::vector<char> data_buffer(length);
        // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
        emscripten::val::module_property("HEAPU8")
                .call<void>("set", data_array, reinterpret_cast<uintptr_t>(data_buffer.data()));
        // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)

        auto reader_result = clp_s::ffi::sfa::ClpArchiveReader::create(std::move(data_buffer));

        if (reader_result.has_error()) {
            auto const error{reader_result.error()};
            auto const err_msg{fmt::format(
                    "Failed to open SFA archive from buffer: {} - {}.",
                    error.category().name(),
                    error.message()
            )};
            SPDLOG_ERROR("{}", err_msg);
            throw ClpFfiJsException{
                    clp::ErrorCode::ErrorCode_Failure,
                    __FILENAME__,
                    __LINE__,
                    err_msg
            };
        }

        return std::unique_ptr<SfaReader>{new SfaReader{std::move(reader_result.value())}};
    }

    [[nodiscard]] auto get_event_count() const -> uint64_t { return m_reader.get_event_count(); }

    [[nodiscard]] auto get_file_names() const -> emscripten::val {
        auto file_names{emscripten::val::array()};
        for (auto const& file_name : m_reader.get_file_names()) {
            file_names.call<void>("push", emscripten::val(file_name));
        }
        return file_names;
    }

    [[nodiscard]] auto get_file_infos() const -> emscripten::val {
        auto source_file_ranges{emscripten::val::array()};
        for (auto const& file_info : m_reader.get_file_infos()) {
            auto entry{emscripten::val::object()};
            entry.set("fileName", emscripten::val(file_info.get_file_name()));
            entry.set("logEventIdxStart", emscripten::val(file_info.get_start_index()));
            entry.set("logEventIdxEnd", emscripten::val(file_info.get_end_index()));
            entry.set("logEventCount", emscripten::val(file_info.get_event_count()));
            source_file_ranges.call<void>("push", entry);
        }
        return source_file_ranges;
    }

private:
    explicit SfaReader(clp_s::ffi::sfa::ClpArchiveReader&& reader) : m_reader(std::move(reader)) {}

    clp_s::ffi::sfa::ClpArchiveReader m_reader;
};
}  // namespace clp_ffi_js::sfa

EMSCRIPTEN_BINDINGS(SfaReader) {
    emscripten::class_<clp_ffi_js::sfa::SfaReader>("ClpSfaReader")
            .class_function(
                    "create",
                    &clp_ffi_js::sfa::SfaReader::create,
                    emscripten::return_value_policy::take_ownership()
            )
            .function("getEventCount", &clp_ffi_js::sfa::SfaReader::get_event_count)
            .function("getFileNames", &clp_ffi_js::sfa::SfaReader::get_file_names)
            .function("getFileInfos", &clp_ffi_js::sfa::SfaReader::get_file_infos);
}
