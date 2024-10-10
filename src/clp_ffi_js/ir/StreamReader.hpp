#ifndef CLP_FFI_JS_IR_STREAM_READER_HPP
#define CLP_FFI_JS_IR_STREAM_READER_HPP

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

#include <clp/ir/types.hpp>
#include <clp/TimestampPattern.hpp>
#include <emscripten/bind.h>
#include <emscripten/val.h>

#include <clp_ffi_js/ir/StreamReaderDataContext.hpp>

namespace clp_ffi_js::ir {
    EMSCRIPTEN_DECLARE_VAL_TYPE(DataArrayTsType);
    EMSCRIPTEN_DECLARE_VAL_TYPE(DecodedResultsTsType);
    EMSCRIPTEN_DECLARE_VAL_TYPE(FilteredLogEventMapTsType);

    /**
     * Class to deserialize and decode Zstandard-compressed CLP IR streams as well as format decoded
     * log events.
     */
    class StreamReader {
    public:
     virtual ~StreamReader() = default;

        [[nodiscard]] static auto create(DataArrayTsType const& data_array) -> std::unique_ptr<StreamReader>;

        /**
         * @return The number of events buffered.
         */
        [[nodiscard]] virtual auto get_num_events_buffered() const -> size_t = 0;

        /**
         * @return The filtered log events map.
         */
        [[nodiscard]] virtual auto get_filtered_log_event_map() const -> FilteredLogEventMapTsType = 0;

        /**
         * Generates a filtered collection from all log events.
         *
         * @param log_level_filter Array of selected log levels
         */
        virtual auto filter_log_events(emscripten::val const &log_level_filter) -> void = 0;

        /**
         * Deserializes all log events in the file. After the stream has been exhausted, it will be
         * deallocated.
         *
         * @return The number of successfully deserialized ("valid") log events.
         */
        [[nodiscard]] virtual auto deserialize_stream() -> size_t = 0;

        /**
         * Decodes log events in the range `[beginIdx, endIdx)` of the filtered or unfiltered
         * (depending on the value of `useFilter`) log events collection.
         *
         * @param begin_idx
         * @param end_idx
         * @param use_filter If true, decode from the filtered log events collection; otherwise, decode
         * from the unfiltered one.
         * @return An array where each element is a decoded log event represented by an array of:
         * - The log event's message
         * - The log event's timestamp as milliseconds since the Unix epoch
         * - The log event's log level as an integer that indexes into `cLogLevelNames`
         * - The log event's number (1-indexed) in the stream
         * @return null if any log event in the range doesn't exist (e.g. the range exceeds the number
         * of log events in the collection).
         */
        [[nodiscard]] virtual auto
        decode_range(size_t begin_idx, size_t end_idx, bool use_filter) const -> DecodedResultsTsType = 0;
    };
} // namespace clp_ffi_js::ir

#endif  // CLP_FFI_JS_IR_STREAM_READER_HPP
