#ifndef CLP_FFI_JS_SFA_SFAREADER_HPP
#define CLP_FFI_JS_SFA_SFAREADER_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <clp_s/ffi/sfa/ClpArchiveReader.hpp>
#include <emscripten/val.h>

#include <clp_ffi_js/binding_types.hpp>

namespace clp_ffi_js::sfa {
EMSCRIPTEN_DECLARE_VAL_TYPE(FileInfoArrayTsType);
EMSCRIPTEN_DECLARE_VAL_TYPE(FilteredLogEventMapTsType);
EMSCRIPTEN_DECLARE_VAL_TYPE(LogEventArrayTsType);
EMSCRIPTEN_DECLARE_VAL_TYPE(NullableLogEventArrayTsType);

using FilteredLogEventsMap = std::optional<std::vector<size_t>>;

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

    [[nodiscard]] auto get_file_names() const -> clp_ffi_js::StringArrayTsType;

    [[nodiscard]] auto get_file_infos() const -> FileInfoArrayTsType;

    [[nodiscard]] auto get_filtered_log_event_map() const -> FilteredLogEventMapTsType;

    void filter_log_events(std::string const& kql_filter, std::string const& log_level_kql_filter);

    void clear_query() { m_filtered_log_event_map.reset(); }

    auto decode() -> void;

    [[nodiscard]] auto decode_all() -> LogEventArrayTsType;

    [[nodiscard]] auto decode_range(size_t begin_idx, size_t end_idx, bool use_filter)
            -> NullableLogEventArrayTsType;

    [[nodiscard]] auto find_nearest_log_event_by_timestamp(int64_t target_timestamp)
            -> clp_ffi_js::NullableLogEventIdx;

private:
    explicit SfaReader(clp_s::ffi::sfa::ClpArchiveReader&& reader) : m_reader(std::move(reader)) {}

    clp_s::ffi::sfa::ClpArchiveReader m_reader;
    FilteredLogEventsMap m_filtered_log_event_map;
};
}  // namespace clp_ffi_js::sfa

#endif  // CLP_FFI_JS_SFA_SFAREADER_HPP
