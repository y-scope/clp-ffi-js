/**
 * Value for `IrStreamType.STRUCTURED`. Matches `StreamType::Structured` in
 * `src/clp_ffi_js/ir/StreamReader.hpp`.
 */
const IR_STREAM_TYPE_STRUCTURED = 0;

/**
 * Value for `IrStreamType.UNSTRUCTURED`. Matches `StreamType::Unstructured` in
 * `src/clp_ffi_js/ir/StreamReader.hpp`.
 */
const IR_STREAM_TYPE_UNSTRUCTURED = 1;

/**
 * Number of events to decode when testing `decodeRange` with a small range from the beginning of
 * the stream. This is an arbitrary small value to avoid processing the entire stream in tests.
 */
const DECODE_CHUNK_SIZE = 5;

/**
 * Number of events to decode when testing filtered results or the last few events in a stream.
 * Smaller than `DECODE_CHUNK_SIZE` to provide variety in test scenarios.
 */
const FILTERED_CHUNK_SIZE = 3;

/**
 * Log level value for INFO logs. Matches `LogLevel::INFO` in `src/clp_ffi_js/constants.hpp`.
 */
const LOG_LEVEL_INFO = 3;

/**
 * Log level value for WARN logs. Matches `LogLevel::WARN` in `src/clp_ffi_js/constants.hpp`.
 */
const LOG_LEVEL_WARN = 4;

/**
 * Log level value for ERROR logs. Matches `LogLevel::ERROR` in `src/clp_ffi_js/constants.hpp`.
 */
const LOG_LEVEL_ERROR = 5;

/**
 * Total number of log events in `structured-cockroachdb.clp.zst`.
 */
const NUM_EVENTS_STRUCTURED_COCKROACHDB = 200000;

/**
 * Total number of log events in `unstructured-yarn.clp.zst`.
 */
const NUM_EVENTS_UNSTRUCTURED_YARN = 375558;

/**
 * Expected number of INFO-level events in `structured-cockroachdb.clp.zst` when `logLevelKey` is
 * extracted.
 */
const NUM_EVENTS_STRUCTURED_COCKROACHDB_INFO = 199974;

/**
 * Expected number of INFO-level events in `unstructured-yarn.clp.zst`.
 */
const NUM_EVENTS_UNSTRUCTURED_YARN_INFO = 301322;

/**
 * Expected number of WARN+ERROR-level events in `unstructured-yarn.clp.zst`.
 */
const NUM_EVENTS_UNSTRUCTURED_YARN_WARN_ERROR = 74236;

/**
 * Expected number of events matching KQL query `"INFO"` in `structured-cockroachdb.clp.zst`.
 */
const NUM_EVENTS_MATCHING_KQL_INFO = 199974;


export {
    DECODE_CHUNK_SIZE,
    FILTERED_CHUNK_SIZE,
    IR_STREAM_TYPE_STRUCTURED,
    IR_STREAM_TYPE_UNSTRUCTURED,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    NUM_EVENTS_MATCHING_KQL_INFO,
    NUM_EVENTS_STRUCTURED_COCKROACHDB,
    NUM_EVENTS_STRUCTURED_COCKROACHDB_INFO,
    NUM_EVENTS_UNSTRUCTURED_YARN,
    NUM_EVENTS_UNSTRUCTURED_YARN_INFO,
    NUM_EVENTS_UNSTRUCTURED_YARN_WARN_ERROR,
};
