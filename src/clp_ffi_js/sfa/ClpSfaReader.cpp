#include <cstdint>
#include <format>
#include <memory>
#include <stdexcept>
#include <string>

#include <clp_s/ffi/sfa/ClpArchiveReader.hpp>
#include <emscripten/bind.h>

namespace clp_ffi_js::sfa {
class ClpSfaReader {
public:
    static auto create(std::string const& archive_path) -> std::unique_ptr<ClpSfaReader> {
        auto reader_result = clp_s::ffi::sfa::ClpArchiveReader::create(archive_path);
        if (reader_result.has_error()) {
            auto const error = reader_result.error();
            throw std::runtime_error{
                    std::format(
                            "Failed to open SFA archive '{}': {}:{}",
                            archive_path,
                            error.category().name(),
                            error.message()
                    )
            };
        }

        return std::unique_ptr<ClpSfaReader>{
                new ClpSfaReader{std::move(reader_result.value())}
        };
    }

    [[nodiscard]] auto get_archive_id() const -> std::string { return m_reader.get_archive_id(); }

    [[nodiscard]] auto get_event_count() const -> uint64_t { return m_reader.get_event_count(); }

private:
    explicit ClpSfaReader(clp_s::ffi::sfa::ClpArchiveReader&& reader) : m_reader(std::move(reader)) {}

    clp_s::ffi::sfa::ClpArchiveReader m_reader;
};
}  // namespace clp_ffi_js::sfa

EMSCRIPTEN_BINDINGS(ClpSfaReader) {
    emscripten::class_<clp_ffi_js::sfa::ClpSfaReader>("ClpSfaReader")
            .constructor(
                    &clp_ffi_js::sfa::ClpSfaReader::create,
                    emscripten::return_value_policy::take_ownership()
            )
            .function("getArchiveId", &clp_ffi_js::sfa::ClpSfaReader::get_archive_id)
            .function("getEventCount", &clp_ffi_js::sfa::ClpSfaReader::get_event_count);
}
