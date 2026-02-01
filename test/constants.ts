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
 * Offset added to `numEvents` to create an out-of-bounds index for testing.
 */
const OUT_OF_BOUNDS_OFFSET = 1;


export {
    DECODE_CHUNK_SIZE,
    FILTERED_CHUNK_SIZE,
    IR_STREAM_TYPE_STRUCTURED,
    IR_STREAM_TYPE_UNSTRUCTURED,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    OUT_OF_BOUNDS_OFFSET,
};
