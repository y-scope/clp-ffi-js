#include "query_methods.hpp"

#include <cstddef>
#include <format>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include <clp/ErrorCode.hpp>
#include <clp/ffi/ir_stream/decoding_methods.hpp>
#include <clp/ffi/ir_stream/Deserializer.hpp>
#include <clp/ffi/ir_stream/search/QueryHandler.hpp>
#include <clp/ffi/KeyValuePairLogEvent.hpp>
#include <clp/ffi/SchemaTree.hpp>
#include <clp/ReaderInterface.hpp>
#include <clp/time_types.hpp>
#include <clp_s/search/kql/kql.hpp>
#include <spdlog/spdlog.h>
#include <ystdlib/error_handling/Result.hpp>

#include <clp_ffi_js/ClpFfiJsException.hpp>
#include <clp_ffi_js/ir/decoding_methods.hpp>

namespace clp_ffi_js::ir {
namespace {
using clp::ffi::ir_stream::IRErrorCode;
using clp::ffi::KeyValuePairLogEvent;
using clp::UtcOffset;

class LogEventIndexIrUnitHandler {
public:
    [[nodiscard]] auto
    handle_log_event([[maybe_unused]] KeyValuePairLogEvent&& log_event, size_t log_event_idx)
            -> IRErrorCode {
        m_deserialized_log_event_indices.push_back(log_event_idx);
        return IRErrorCode::IRErrorCode_Success;
    }

    [[nodiscard]] static auto handle_utc_offset_change(
            [[maybe_unused]] UtcOffset utc_offset_old,
            [[maybe_unused]] UtcOffset utc_offset_new
    ) -> IRErrorCode {
        return IRErrorCode::IRErrorCode_Decode_Error;
    }

    [[nodiscard]] static auto handle_schema_tree_node_insertion(
            [[maybe_unused]] bool is_auto_generated,
            [[maybe_unused]] clp::ffi::SchemaTree::NodeLocator schema_tree_node_locator,
            [[maybe_unused]] std::shared_ptr<clp::ffi::SchemaTree const> const& schema_tree
    ) -> IRErrorCode {
        return IRErrorCode::IRErrorCode_Success;
    }

    [[nodiscard]] static auto handle_end_of_stream() -> IRErrorCode {
        return IRErrorCode::IRErrorCode_Success;
    }

    [[nodiscard]] auto get_deserialized_log_event_indices() const -> std::vector<size_t> const& {
        return m_deserialized_log_event_indices;
    }

private:
    std::vector<size_t> m_deserialized_log_event_indices;
};

auto trivial_new_projected_schema_tree_node_callback(
        [[maybe_unused]] bool is_auto_generated,
        [[maybe_unused]] clp::ffi::SchemaTree::Node::id_t node_id,
        [[maybe_unused]] std::pair<std::string_view, size_t> projected_key_path_and_index
) -> ystdlib::error_handling::Result<void> {
    return ystdlib::error_handling::success();
}
}  // namespace

[[nodiscard]] auto
collect_matched_log_event_indices(clp::ReaderInterface& reader, std::string const& query_string)
        -> std::vector<size_t> {
    std::istringstream query_string_stream{query_string};
    auto query_handler_result{
            clp::ffi::ir_stream::search::QueryHandler<
                    decltype(&trivial_new_projected_schema_tree_node_callback)>::
                    create(&trivial_new_projected_schema_tree_node_callback,
                           clp_s::search::kql::parse_kql_expression(query_string_stream),
                           {},
                           false)
    };

    if (query_handler_result.has_error()) {
        auto const error_code{query_handler_result.error()};
        throw ClpFfiJsException{
                clp::ErrorCode::ErrorCode_Failure,
                __FILENAME__,
                __LINE__,
                std::format(
                        "Failed to create query handler: {} {}",
                        error_code.category().name(),
                        error_code.message()
                )
        };
    }

    auto deserializer_result{clp::ffi::ir_stream::make_deserializer(
            reader,
            LogEventIndexIrUnitHandler{},
            std::move(query_handler_result.value())
    )};

    if (deserializer_result.has_error()) {
        auto const error_code{deserializer_result.error()};
        throw ClpFfiJsException{
                clp::ErrorCode::ErrorCode_Failure,
                __FILENAME__,
                __LINE__,
                std::format(
                        "Failed to create deserializer: {} {}",
                        error_code.category().name(),
                        error_code.message()
                )
        };
    }

    auto& deserializer{deserializer_result.value()};
    deserialize_log_events(deserializer, reader);
    return deserializer.get_ir_unit_handler().get_deserialized_log_event_indices();
}
}  // namespace clp_ffi_js::ir
