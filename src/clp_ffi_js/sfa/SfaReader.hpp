#ifndef CLP_FFI_JS_SFA_SFAREADER_HPP
#define CLP_FFI_JS_SFA_SFAREADER_HPP

#include <cstdint>
#include <memory>
#include <utility>

#include <emscripten/val.h>

#include <clp_s/ffi/sfa/ClpArchiveReader.hpp>

#include <clp_ffi_js/binding_types.hpp>

namespace clp_ffi_js::sfa {
class SfaReader {
public:
    /**
     * Creates an `SfaReader` from the given data array.
     *
     * @param data_array An array containing an SFA archive.
     * @return The created instance.
     * @throw std::runtime_error if the archive cannot be opened.
     */
    [[nodiscard]] static auto create(clp_ffi_js::DataArrayTsType const& data_array)
            -> std::unique_ptr<SfaReader>;

    [[nodiscard]] auto get_event_count() const -> uint64_t { return m_reader.get_event_count(); }
    [[nodiscard]] auto get_file_names() const -> emscripten::val;
    [[nodiscard]] auto get_file_infos() const -> emscripten::val;

private:
    explicit SfaReader(clp_s::ffi::sfa::ClpArchiveReader&& reader) : m_reader(std::move(reader)) {}

    clp_s::ffi::sfa::ClpArchiveReader m_reader;
};
}  // namespace clp_ffi_js::sfa

#endif  // CLP_FFI_JS_SFA_SFAREADER_HPP
