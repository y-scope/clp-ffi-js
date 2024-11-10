#ifndef CLP_FFI_JS_IR_STRUCTUREDIRSTREAMREADER_HPP
#define CLP_FFI_JS_IR_STRUCTUREDIRSTREAMREADER_HPP

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <clp/Array.hpp>
#include <clp/ffi/ir_stream/decoding_methods.hpp>
#include <clp/ffi/ir_stream/Deserializer.hpp>
#include <clp/ffi/KeyValuePairLogEvent.hpp>
#include <clp/ffi/SchemaTree.hpp>
#include <clp/time_types.hpp>
#include <emscripten/val.h>
#include <spdlog/spdlog.h>

#include <clp_ffi_js/ir/LogEventWithFilterData.hpp>
#include <clp_ffi_js/ir/StreamReader.hpp>
#include <clp_ffi_js/ir/StreamReaderDataContext.hpp>

namespace clp_ffi_js::ir {
using schema_tree_node_id_t = std::optional<clp::ffi::SchemaTree::Node::id_t>;
using StructuredLogEvent = clp::ffi::KeyValuePairLogEvent;

/**
 * Class that implements the `clp::ffi::ir_stream::IrUnitHandlerInterface` to buffer log events and
 * determine the schema-tree node ID of the timestamp kv-pair.
 */
class IrUnitHandler {
public:
    /**
     * @param deserialized_log_events The vector in which to store deserialized log events.
     * @param log_level_key Key name of schema-tree node that contains the authoritative log level.
     * @param timestamp_key Key name of schema-tree node that contains the authoritative timestamp.
     */
    IrUnitHandler(
            std::shared_ptr<std::vector<LogEventWithFilterData<StructuredLogEvent>>>
                    deserialized_log_events,
            std::string log_level_key,
            std::string timestamp_key
    )
            : m_log_level_key{std::move(log_level_key)},
              m_timestamp_key{std::move(timestamp_key)},
              m_deserialized_log_events{std::move(deserialized_log_events)} {}

    // Methods implementing `clp::ffi::ir_stream::IrUnitHandlerInterface`.
    /**
     * Buffers the log event.
     * @param log_event
     * @return IRErrorCode::IRErrorCode_Success
     */
    [[nodiscard]] auto handle_log_event(StructuredLogEvent&& log_event
    ) -> clp::ffi::ir_stream::IRErrorCode;

    /**
     * @param utc_offset_old
     * @param utc_offset_new
     * @return IRErrorCode::IRErrorCode_Success
     */
    [[nodiscard]] static auto handle_utc_offset_change(
            [[maybe_unused]] clp::UtcOffset utc_offset_old,
            [[maybe_unused]] clp::UtcOffset utc_offset_new
    ) -> clp::ffi::ir_stream::IRErrorCode {
        SPDLOG_WARN("UTC offset change packets aren't handled currently.");

        return clp::ffi::ir_stream::IRErrorCode::IRErrorCode_Success;
    }

    /**
     * Saves the node's ID if it corresponds to events' authoritative timestamp kv-pair.
     * @param schema_tree_node_locator
     * @return IRErrorCode::IRErrorCode_Success
     */
    [[nodiscard]] auto handle_schema_tree_node_insertion(
            clp::ffi::SchemaTree::NodeLocator schema_tree_node_locator
    ) -> clp::ffi::ir_stream::IRErrorCode;

    /**
     * @return IRErrorCode::IRErrorCode_Success
     */
    [[nodiscard]] static auto handle_end_of_stream() -> clp::ffi::ir_stream::IRErrorCode {
        return clp::ffi::ir_stream::IRErrorCode::IRErrorCode_Success;
    }

    // Methods

    /**
     * @return The schema-tree node ID associated with events' authoritative timestamp key.
     */
    [[nodiscard]] auto get_timestamp_node_id() const -> schema_tree_node_id_t {
        return m_timestamp_node_id;
    }

private:
    // Variables
    std::string m_log_level_key;
    std::string m_timestamp_key;

    clp::ffi::SchemaTree::Node::id_t m_current_node_id{clp::ffi::SchemaTree::cRootId};

    schema_tree_node_id_t m_timestamp_node_id;
    schema_tree_node_id_t m_log_level_node_id;

    // TODO: Technically, we don't need to use a `shared_ptr` since the parent stream reader will
    // have a longer lifetime than this class. Instead, we could use `gsl::not_null` once we add
    // `gsl` into the project.
    std::shared_ptr<std::vector<LogEventWithFilterData<StructuredLogEvent>>>
            m_deserialized_log_events;
};

using StructuredIrDeserializer = clp::ffi::ir_stream::Deserializer<IrUnitHandler>;

/**
 * Class to deserialize and decode Zstd-compressed CLP structured IR streams, as well as format
 * decoded log events.
 */
class StructuredIrStreamReader : public StreamReader {
public:
    /**
     * @param zstd_decompressor A decompressor for an IR stream, where the read head of the stream
     * is just after the stream's encoding type.
     * @param data_array The array backing `zstd_decompressor`.
     * @param reader_options
     * @return The created instance.
     * @throw ClpFfiJsException if any error occurs.
     */
    [[nodiscard]] static auto create(
            std::unique_ptr<ZstdDecompressor>&& zstd_decompressor,
            clp::Array<char> data_array,
            ReaderOptions const& reader_options
    ) -> StructuredIrStreamReader;

    // Destructor
    ~StructuredIrStreamReader() override = default;

    // Disable copy constructor and assignment operator
    StructuredIrStreamReader(StructuredIrStreamReader const&) = delete;
    auto operator=(StructuredIrStreamReader const&) -> StructuredIrStreamReader& = delete;

    // Define default move constructor
    StructuredIrStreamReader(StructuredIrStreamReader&&) = default;
    // Delete move assignment operator since it's also disabled in `clp::ir::LogEventDeserializer`.
    auto operator=(StructuredIrStreamReader&&) -> StructuredIrStreamReader& = delete;

    [[nodiscard]] auto get_ir_stream_type() const -> StreamType override {
        return StreamType::Structured;
    }

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
            std::shared_ptr<std::vector<LogEventWithFilterData<StructuredLogEvent>>>
                    deserialized_log_events
    );

    // Variables
    std::shared_ptr<std::vector<LogEventWithFilterData<StructuredLogEvent>>>
            m_deserialized_log_events;
    std::unique_ptr<StreamReaderDataContext<StructuredIrDeserializer>> m_stream_reader_data_context;
    FilteredLogEventsMap m_filtered_log_event_map;
};
}  // namespace clp_ffi_js::ir

#endif  // CLP_FFI_JS_IR_STRUCTUREDIRSTREAMREADER_HPP
