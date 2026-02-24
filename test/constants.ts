import type {ReaderOptions} from "./types.js";


/**
 * Default reader options used when creating a `ClpStreamReader` in tests.
 */
const DEFAULT_READER_OPTIONS: ReaderOptions = {
    logLevelKey: null,
    timestampKey: null,
    utcOffsetKey: null,
};

/**
 * Filesystem URL for the local test data directory.
 */
const TEST_DATA_DIR_URL = new URL("data/", import.meta.url);
/**
 * Browser-served base path for test data files.
 */
const TEST_DATA_WEB_BASE_PATH = "/test/data/";

/**
 * Remote sources used to download fixture files during global setup.
 */
const TEST_DATA_URLS: Readonly<Record<string, string>> = {
    "structured-cockroachdb.clp.zst":
        "https://yscope.s3.us-east-2.amazonaws.com/sample-logs/cockroachdb.clp.zst",
    "unstructured-yarn.clp.zst":
        "https://yscope.s3.us-east-2.amazonaws.com/sample-logs/" +
        "yarn-ubuntu-resourcemanager-ip-172-31-17-135.log.1.clp.zst",
};

/**
 * Default path to the Node.js CLP module bundle for tests.
 */
const DEFAULT_NODE_MODULE_PATH = "../dist/ClpFfiJs-node.js";

/**
 * Default path to the worker/browser CLP module bundle for tests.
 */
const DEFAULT_WORKER_MODULE_PATH = "../dist/ClpFfiJs-worker.js";

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


export {
    DEFAULT_NODE_MODULE_PATH,
    DEFAULT_READER_OPTIONS,
    DEFAULT_WORKER_MODULE_PATH,
    IR_STREAM_TYPE_STRUCTURED,
    IR_STREAM_TYPE_UNSTRUCTURED,
    TEST_DATA_DIR_URL,
    TEST_DATA_URLS,
    TEST_DATA_WEB_BASE_PATH,
};
