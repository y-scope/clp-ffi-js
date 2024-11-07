#ifndef CLP_FFI_JS_IR_STRUCTUREDIRSTREAMREADER_HPP
#define CLP_FFI_JS_IR_STRUCTUREDIRSTREAMREADER_HPP

#include <Array.hpp>
#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

#include <clp/ffi/ir_stream/Deserializer.hpp>
#include <clp/ir/LogEventDeserializer.hpp>
#include <clp/ir/types.hpp>
#include <clp/TimestampPattern.hpp>
#include <emscripten/val.h>
#include <spdlog/spdlog.h>

#include <clp_ffi_js/ir/LogEventWithLevel.hpp>
#include <clp_ffi_js/ir/StreamReader.hpp>
#include <clp_ffi_js/ir/StreamReaderDataContext.hpp>

namespace clp_ffi_js::ir {
using parsed_tree_node_id_t = std::optional<clp::ffi::SchemaTree::Node::id_t>;

/**
 * Class to handle deserialized IR units.
 */
class IrUnitHandler {
public:
    IrUnitHandler(
            std::vector<clp::ffi::KeyValuePairLogEvent>& deserialized_log_events,
            std::string log_level_key,
            std::string timestamp_key
    )
            : m_deserialized_log_events{deserialized_log_events},
              m_log_level_key{std::move(log_level_key)},
              m_timestamp_key{std::move(timestamp_key)} {}

    // Implements `clp::ffi::ir_stream::IrUnitHandlerInterface` interface
    [[nodiscard]] auto handle_log_event(clp::ffi::KeyValuePairLogEvent&& log_event
    ) -> clp::ffi::ir_stream::IRErrorCode {
        m_deserialized_log_events.emplace_back(std::move(log_event));

        return clp::ffi::ir_stream::IRErrorCode::IRErrorCode_Success;
    }

    [[nodiscard]] static auto handle_utc_offset_change(
            [[maybe_unused]] clp::UtcOffset utc_offset_old,
            [[maybe_unused]] clp::UtcOffset utc_offset_new
    ) -> clp::ffi::ir_stream::IRErrorCode {
        SPDLOG_WARN("UTC offset change packets are currently not handled.");

        return clp::ffi::ir_stream::IRErrorCode::IRErrorCode_Success;
    }

    [[nodiscard]] auto handle_schema_tree_node_insertion(
            [[maybe_unused]] clp::ffi::SchemaTree::NodeLocator schema_tree_node_locator
    ) -> clp::ffi::ir_stream::IRErrorCode {
        ++m_current_node_id;
        auto const& key_name{schema_tree_node_locator.get_key_name()};

        if (m_log_level_key == key_name) {
            m_level_node_id.emplace(m_current_node_id);
        } else if (m_timestamp_key == key_name) {
            m_timestamp_node_id.emplace(m_current_node_id);
        }

        return clp::ffi::ir_stream::IRErrorCode::IRErrorCode_Success;
    }

    // FIXME: do i need this?
    [[nodiscard]] static auto handle_end_of_stream() -> clp::ffi::ir_stream::IRErrorCode {
        return clp::ffi::ir_stream::IRErrorCode::IRErrorCode_Success;
    }

    // Methods
    [[nodiscard]] auto get_deserialized_log_events(
    ) const -> std::vector<clp::ffi::KeyValuePairLogEvent> const& {
        return m_deserialized_log_events;
    }

    [[nodiscard]] auto get_level_node_id() const -> parsed_tree_node_id_t {
        return m_level_node_id;
    }

    [[nodiscard]] auto get_timestamp_node_id() const -> parsed_tree_node_id_t {
        return m_timestamp_node_id;
    }

private:
    std::string m_log_level_key;
    std::string m_timestamp_key;

    // the root node has id=0
    clp::ffi::SchemaTree::Node::id_t m_current_node_id;
    parsed_tree_node_id_t m_level_node_id;
    parsed_tree_node_id_t m_timestamp_node_id;

    std::vector<clp::ffi::KeyValuePairLogEvent>& m_deserialized_log_events;
    bool m_is_complete{false};
};

using StructuredIrDeserializer = clp::ffi::ir_stream::Deserializer<IrUnitHandler>;

/**
 * Class to deserialize and decode Zstd-compressed CLP structured IR streams, as well as format
 * decoded log events.
 */
class StructuredIrStreamReader : public StreamReader {
public:
    // Destructor
    ~StructuredIrStreamReader() override = default;

    // Disable copy constructor and assignment operator
    StructuredIrStreamReader(StructuredIrStreamReader const&) = delete;
    auto operator=(StructuredIrStreamReader const&) -> StructuredIrStreamReader& = delete;

    // Define default move constructor
    StructuredIrStreamReader(StructuredIrStreamReader&&) = default;
    // Delete move assignment operator since it's also disabled in `clp::ir::LogEventDeserializer`.
    auto operator=(StructuredIrStreamReader&&) -> StructuredIrStreamReader& = delete;

    /**
     * @param zstd_decompressor A decompressor for an IR stream, where the read head of the stream
     * is just after the stream's encoding type.
     * @param data_array The array backing `zstd_decompressor`.
     * @return The created instance.
     * @throw ClpFfiJsException if any error occurs.
     */
    [[nodiscard]] static auto create(
            std::unique_ptr<ZstdDecompressor>&& zstd_decompressor,
            clp::Array<char> data_array,
            ReaderOptions const& reader_options
    ) -> StructuredIrStreamReader;

    [[nodiscard]] auto get_num_events_buffered() const -> size_t override;

    [[nodiscard]] auto get_filtered_log_event_map() const -> FilteredLogEventMapTsType override;

    void filter_log_events(LogLevelFilterTsType const& log_level_filter) override;

    /**
     * @see StreamReader::deserialize_stream
     *
     * After the stream has been exhausted, it will be deallocated.
     *
     * @return @see StreamReader::deserialize_stream
     */
    [[nodiscard]] auto deserialize_stream() -> size_t override;

    [[nodiscard]] auto decode_range(size_t begin_idx, size_t end_idx, bool use_filter) const
            -> DecodedResultsTsType override;

private:
    // Constructor
    explicit StructuredIrStreamReader(
            StreamReaderDataContext<StructuredIrDeserializer>&& stream_reader_data_context,
            std::shared_ptr<std::vector<clp::ffi::KeyValuePairLogEvent>> deserialized_log_events
    );

    // Variables
    std::shared_ptr<std::vector<clp::ffi::KeyValuePairLogEvent>> m_deserialized_log_events;
    std::unique_ptr<StreamReaderDataContext<StructuredIrDeserializer>> m_stream_reader_data_context;

    parsed_tree_node_id_t m_level_node_id;
    parsed_tree_node_id_t m_timestamp_node_id;
};
}  // namespace clp_ffi_js::ir

#endif  // CLP_FFI_JS_IR_STRUCTUREDIRSTREAMREADER_HPP
