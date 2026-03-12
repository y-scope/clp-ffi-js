#include <cstdint>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>

#include <clp_s/ffi/sfa/ClpArchiveReader.hpp>
#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <spdlog/spdlog.h>

#include <clp_ffi_js/ir/StreamReader.hpp>

namespace clp_ffi_js::sfa {
class SfaReader {
public:
    static auto create(ir::DataArrayTsType const& data_array) -> std::unique_ptr<SfaReader> {
        auto const length{data_array["length"].as<size_t>()};
        SPDLOG_INFO("SfaReader::create: got buffer of length={}", length);

        // Copy array from JavaScript to C++.
        std::vector<char> data_buffer(length);
        // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
        emscripten::val::module_property("HEAPU8").call<void>(
                "set",
                data_array,
                reinterpret_cast<uintptr_t>(data_buffer.data()));
        // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)

        auto reader_result = clp_s::ffi::sfa::ClpArchiveReader::create(std::move(data_buffer));

        if (reader_result.has_error()) {
            auto const error = reader_result.error();
            throw std::runtime_error{"Failed to open SFA archive from buffer"};
        }

        return std::unique_ptr<SfaReader>{new SfaReader{std::move(reader_result.value())}};
    }

    [[nodiscard]] auto get_event_count() const -> uint64_t { return m_reader.get_event_count(); }

private:
    SfaReader(clp_s::ffi::sfa::ClpArchiveReader&& reader) : m_reader(std::move(reader)) {}

    clp_s::ffi::sfa::ClpArchiveReader m_reader;
};
}  // namespace clp_ffi_js::sfa

EMSCRIPTEN_BINDINGS(SfaReader) {
    emscripten::class_<clp_ffi_js::sfa::SfaReader>("ClpSfaReader")
            .class_function("create",
                            &clp_ffi_js::sfa::SfaReader::create,
                            emscripten::return_value_policy::take_ownership())
            .function("getEventCount", &clp_ffi_js::sfa::SfaReader::get_event_count);
}
