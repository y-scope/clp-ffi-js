#include <cstdint>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>

#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <fmt/core.h>
#include <spdlog/spdlog.h>
#include <clp_s/ffi/sfa/ClpArchiveReader.hpp>
#include <clp_ffi_js/ir/StreamReader.hpp>

namespace clp_ffi_js::sfa {
class SfaReader {
public:
    static auto create(ir::DataArrayTsType const& data_array,
                       std::string const& archive_id) -> std::unique_ptr<SfaReader> {
        auto const length{data_array["length"].as<size_t>()};
        SPDLOG_INFO("SfaReader::create: got buffer of length={}", length);

        // Copy array from JavaScript to C++.
        ystdlib::containers::Array<char> data_buffer(length);
        // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
        emscripten::val::module_property("HEAPU8").call<void>(
                "set",
                data_array,
                reinterpret_cast<uintptr_t>(data_buffer.data()));
        // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)

        auto reader_result = clp_s::ffi::sfa::ClpArchiveReader::create(
                std::span<char const>{data_buffer.data(), data_buffer.size()},
                archive_id);

        if (reader_result.has_error()) {
            auto const error = reader_result.error();
            auto const message
                    = fmt::format("Failed to open SFA archive from buffer: error_category={}, "
                                  "error={}",
                                  error.category().name(),
                                  error.message());
            throw std::runtime_error{message};
        }

        return std::unique_ptr<SfaReader>{
                new SfaReader{std::move(reader_result.value()), std::move(data_buffer)}};
    }

    [[nodiscard]] auto get_archive_id() const -> std::string {
        auto result = m_reader.get_archive_id();
        if (result.has_error()) {
            auto const error = result.error();
            auto const message = fmt::format("Failed to get archive id: error_category={}, error={}",
                                             error.category().name(),
                                             error.message());
            throw std::runtime_error{message};
        }
        return std::move(result).value();
    }

    [[nodiscard]] auto get_event_count() const -> uint64_t {
        return m_reader.get_event_count();
    }

    [[nodiscard]] auto get_file_names() const -> emscripten::val {
        auto file_names{emscripten::val::array()};
        for (auto const& file_name : m_reader.get_file_names()) {
            file_names.call<void>("push", emscripten::val(file_name));
        }
        return file_names;
    }

    [[nodiscard]] auto get_source_file_ranges() const -> emscripten::val {
        auto source_file_ranges{emscripten::val::array()};
        for (auto const& file_info : m_reader.get_source_file_ranges()) {
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
    SfaReader(clp_s::ffi::sfa::ClpArchiveReader&& reader,
                 ystdlib::containers::Array<char>&& data_buffer)
            : m_reader(std::move(reader)),
              m_data_buffer(std::move(data_buffer)) {}

    clp_s::ffi::sfa::ClpArchiveReader m_reader;

    ystdlib::containers::Array<char> m_data_buffer;
};
}  // namespace clp_ffi_js::sfa

EMSCRIPTEN_BINDINGS(SfaReader) {
    emscripten::class_<clp_ffi_js::sfa::SfaReader>("ClpSfaReader")
            .class_function("create",
                            &clp_ffi_js::sfa::SfaReader::create,
                            emscripten::return_value_policy::take_ownership())
            .function("getArchiveId", &clp_ffi_js::sfa::SfaReader::get_archive_id)
            .function("getEventCount", &clp_ffi_js::sfa::SfaReader::get_event_count)
            .function("getFileNames", &clp_ffi_js::sfa::SfaReader::get_file_names)
            .function(
                    "getSourceFileRanges",
                    &clp_ffi_js::sfa::SfaReader::get_source_file_ranges
            );
}
