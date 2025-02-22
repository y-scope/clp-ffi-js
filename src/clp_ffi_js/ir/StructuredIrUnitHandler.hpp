#ifndef CLP_FFI_JS_IR_STRUCTUREDIRUNITHANDLER_HPP
#define CLP_FFI_JS_IR_STRUCTUREDIRUNITHANDLER_HPP

#include <algorithm>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <clp/ffi/ir_stream/decoding_methods.hpp>
#include <clp/ffi/KeyValuePairLogEvent.hpp>
#include <clp/ffi/SchemaTree.hpp>
#include <clp/ir/types.hpp>
#include <clp/time_types.hpp>

#include <clp_ffi_js/constants.hpp>
#include <clp_ffi_js/ir/LogEventWithFilterData.hpp>

namespace clp_ffi_js::ir {
/**
 * Class that implements the `clp::ffi::ir_stream::IrUnitHandlerInterface` to buffer log events and
 * determine the schema-tree node IDs of the log level and timestamp kv-pairs.
 */
class StructuredIrUnitHandler {
public:
    // Types
    /**
     * Class to represent a full branch from the root to a leaf node in a schema tree.
     * A branch is uniquely identified by the sequence of key names along the path and the type of
     * the leaf node. All non-leaf nodes are implicitly of type `Obj`.
     */
    class SchemaTreeFullBranch {
    public:
        // Constructor
        /**
         * @param is_auto_gen
         * @param root_to_leaf_path
         * @param leaf_type
         */
        SchemaTreeFullBranch(
                bool is_auto_gen,
                std::vector<std::string> root_to_leaf_path,
                clp::ffi::SchemaTree::Node::Type leaf_type
        )
                : m_is_auto_generated{is_auto_gen},
                  m_leaf_to_root_path{std::move(root_to_leaf_path)},
                  m_leaf_type{leaf_type} {
            std::ranges::reverse(m_leaf_to_root_path);
        }

        // Default move constructor and assignment operator
        SchemaTreeFullBranch(SchemaTreeFullBranch&&) = default;
        auto operator=(SchemaTreeFullBranch&&) -> SchemaTreeFullBranch& = default;

        // Delete copy constructor and assignment operator
        SchemaTreeFullBranch(SchemaTreeFullBranch const&) = delete;
        auto operator=(SchemaTreeFullBranch const&) -> SchemaTreeFullBranch& = delete;

        // Destructor
        ~SchemaTreeFullBranch() = default;

        /**
         * @return Whether this branch belongs to the auto-generated schema tree.
         */
        [[nodiscard]] auto is_auto_generated() const -> bool { return m_is_auto_generated; }

        /**
         * @param schema_tree
         * @param leaf_locator
         * @return Whether the underlying full branch matches the branch from the `schema_tree`'s
         * root to the leaf located by `leaf_locator`.
         */
        [[nodiscard]] auto match(
                clp::ffi::SchemaTree const& schema_tree,
                clp::ffi::SchemaTree::NodeLocator const& leaf_locator
        ) const -> bool;

    private:
        bool m_is_auto_generated;
        std::vector<std::string> m_leaf_to_root_path;
        clp::ffi::SchemaTree::Node::Type m_leaf_type;
    };

    // Constructors
    /**
     * @param deserialized_log_events The vector in which to store deserialized log events.
     * @param log_level_key Key name of schema-tree node that contains the authoritative log level.
     * @param timestamp_key Key name of schema-tree node that contains the authoritative timestamp.
     */
    StructuredIrUnitHandler(
            std::shared_ptr<std::vector<LogEventWithFilterData<StructuredLogEvent>>>
                    deserialized_log_events,
            SchemaTreeFullBranch log_level_full_branch,
            SchemaTreeFullBranch timestamp_full_branch
    )
            : m_log_level_full_branch{std::move(log_level_full_branch)},
              m_timestamp_full_branch{std::move(timestamp_full_branch)},
              m_deserialized_log_events{std::move(deserialized_log_events)} {}

    // Methods implementing `clp::ffi::ir_stream::IrUnitHandlerInterface`.
    /**
     * Buffers the log event with filter data extracted.
     * @param log_event
     * @return IRErrorCode::IRErrorCode_Success
     */
    [[nodiscard]] auto handle_log_event(StructuredLogEvent&& log_event
    ) -> clp::ffi::ir_stream::IRErrorCode;

    /**
     * Dummy implementation that does nothing but conforms to the interface.
     * @param utc_offset_old
     * @param utc_offset_new
     * @return IRErrorCode::IRErrorCode_Success
     */
    [[nodiscard]] static auto handle_utc_offset_change(
            [[maybe_unused]] clp::UtcOffset utc_offset_old,
            [[maybe_unused]] clp::UtcOffset utc_offset_new
    ) -> clp::ffi::ir_stream::IRErrorCode;

    /**
     * Saves the node's ID if it corresponds to events' authoritative log level or timestamp
     * kv-pair.
     * @param is_auto_generated
     * @param schema_tree_node_locator
     * @param schema_tree
     * @return IRErrorCode::IRErrorCode_Success
     */
    [[nodiscard]] auto handle_schema_tree_node_insertion(
            bool is_auto_generated,
            clp::ffi::SchemaTree::NodeLocator schema_tree_node_locator,
            std::shared_ptr<clp::ffi::SchemaTree const> const& schema_tree
    ) -> clp::ffi::ir_stream::IRErrorCode;

    /**
     * Dummy implementation that does nothing but conforms to the interface.
     * @return IRErrorCode::IRErrorCode_Success
     */
    [[nodiscard]] static auto handle_end_of_stream() -> clp::ffi::ir_stream::IRErrorCode;

private:
    // Methods
    /**
     * @param id_value_pairs
     * @return Forwards `parse_log_level_from_value`'s return values on success.
     * @return LogLevel::None by default if:
     * - `m_optional_log_level_node_id` is unset.
     * - `m_optional_log_level_node_id` is set but not appearing in the given node-id-value pairs.
     * - `parse_log_level_from_value` fails.
     */
    [[nodiscard]] auto get_log_level(StructuredLogEvent::NodeIdValuePairs const& id_value_pairs
    ) const -> LogLevel;

    /**
     * @param id_value_pairs
     * @return Timestamp from node with ID `m_optional_timestamp_node_id` on success.
     * @return 0 by default if:
     * - `m_optional_timestamp_id` is unset.
     * - `m_optional_timestamp_id` is set but not appearing in the given node-id-value pairs.
     * - The value is not a valid integer.
     */
    [[nodiscard]] auto get_timestamp(StructuredLogEvent::NodeIdValuePairs const& id_value_pairs
    ) const -> clp::ir::epoch_time_ms_t;

    // Variables
    SchemaTreeFullBranch m_log_level_full_branch;
    SchemaTreeFullBranch m_timestamp_full_branch;

    std::optional<clp::ffi::SchemaTree::Node::id_t> m_optional_log_level_node_id;
    std::optional<clp::ffi::SchemaTree::Node::id_t> m_optional_timestamp_node_id;

    // TODO: Technically, we don't need to use a `shared_ptr` since the parent stream reader will
    // have a longer lifetime than this class. Instead, we could use `gsl::not_null` once we add
    // `gsl` into the project.
    std::shared_ptr<std::vector<LogEventWithFilterData<StructuredLogEvent>>>
            m_deserialized_log_events;
};
}  // namespace clp_ffi_js::ir

#endif  // CLP_FFI_JS_IR_STRUCTUREDIRUNITHANDLER_HPP
