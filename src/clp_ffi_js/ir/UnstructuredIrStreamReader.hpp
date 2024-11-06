#ifndef CLP_FFI_JS_IR_UNSTRUCTUREDUnstructuredIrStreamReader_HPP
#define CLP_FFI_JS_IR_UNSTRUCTUREDUnstructuredIrStreamReader_HPP

#include <Array.hpp>
#include <cstddef>
#include <memory>
#include <optional>
#include <streaming_compression/zstd/Decompressor.hpp>
#include <vector>

#include <clp/ir/types.hpp>
#include <clp/TimestampPattern.hpp>
#include <emscripten/val.h>

#include <clp_ffi_js/ir/LogEventWithLevel.hpp>
#include <clp_ffi_js/ir/StreamReader.hpp>
#include <clp_ffi_js/ir/StreamReaderDataContext.hpp>

namespace clp_ffi_js::ir {
using clp::ir::four_byte_encoded_variable_t;

/**
 * Mapping between an index in the filtered log events collection to an index in the unfiltered
 * log events collection.
 */
using FilteredLogEventsMap = std::optional<std::vector<size_t>>;

/**
 * Class to deserialize and decode Zstd-compressed CLP unstructured IR streams, as well as format
 * decoded log events.
 */
class UnstructuredIrStreamReader : public StreamReader {
    friend StreamReader;

public:
    // Destructor
    ~UnstructuredIrStreamReader() override = default;

    // Disable copy constructor and assignment operator
    UnstructuredIrStreamReader(UnstructuredIrStreamReader const&) = delete;
    auto operator=(UnstructuredIrStreamReader const&) -> UnstructuredIrStreamReader& = delete;

    // Define default move constructor
    UnstructuredIrStreamReader(UnstructuredIrStreamReader&&) = default;
    // Delete move assignment operator since it's also disabled in `clp::ir::LogEventDeserializer`.
    auto operator=(UnstructuredIrStreamReader&&) -> UnstructuredIrStreamReader& = delete;

    [[nodiscard]] static auto create(
            std::unique_ptr<ZstdDecompressor>&& zstd_decompressor,
            clp::Array<char>&& data_array
    ) -> std::unique_ptr<StreamReader>;

    [[nodiscard]] auto get_num_events_buffered() const -> size_t override;

    [[nodiscard]] auto get_filtered_log_event_map() const -> FilteredLogEventMapTsType override;

    void filter_log_events(LogLevelFilterTsType const& log_level_filter) override;

    [[nodiscard]] auto deserialize_stream() -> size_t override;

    [[nodiscard]] auto decode_range(size_t begin_idx, size_t end_idx, bool use_filter) const
            -> DecodedResultsTsType override;

private:
    // Constructor
    explicit UnstructuredIrStreamReader(
            StreamReaderDataContext<four_byte_encoded_variable_t>&& stream_reader_data_context
    );

    // Methods
    [[nodiscard]] static auto create_data_context(
            std::unique_ptr<clp::streaming_compression::zstd::Decompressor>&& zstd_decompressor,
            clp::Array<char>&& data_buffer
    ) -> StreamReaderDataContext<four_byte_encoded_variable_t>;

    // Variables
    std::vector<LogEventWithLevel<four_byte_encoded_variable_t>> m_encoded_log_events;
    std::unique_ptr<StreamReaderDataContext<four_byte_encoded_variable_t>>
            m_stream_reader_data_context;
    FilteredLogEventsMap m_filtered_log_event_map;
    clp::TimestampPattern m_ts_pattern;
};
}  // namespace clp_ffi_js::ir

#endif  // CLP_FFI_JS_IR_UNSTRUCTUREDUnstructuredIrStreamReader_HPP
