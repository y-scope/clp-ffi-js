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

auto SfaReader::create(DataArrayTsType const& data_array) -> std::unique_ptr<SfaReader> {
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

    auto reader_result{clp_s::ffi::sfa::ClpArchiveReader::create(std::move(data_buffer))};

    if (reader_result.has_error()) {
        auto const error{reader_result.error()};
        auto const err_msg{fmt::format("Failed to open SFA archive from buffer: {} - {}.",
                                       error.category().name(),
                                       error.message())};
        SPDLOG_ERROR("{}", err_msg);
        throw std::runtime_error{err_msg};
    }

    return std::unique_ptr<SfaReader>{new SfaReader{std::move(reader_result.value())}};
}
}  // namespace clp_ffi_js::sfa

EMSCRIPTEN_BINDINGS(SfaReader) {
    emscripten::class_<clp_ffi_js::sfa::SfaReader>("ClpSfaReader")
            .constructor(&clp_ffi_js::sfa::SfaReader::create,
                         emscripten::return_value_policy::take_ownership())
            .function("getEventCount", &clp_ffi_js::sfa::SfaReader::get_event_count);
}
