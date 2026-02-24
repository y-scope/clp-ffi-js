import type {ReaderOptions} from "./types.js";


const DEFAULT_READER_OPTIONS: ReaderOptions = {
    logLevelKey: null,
    timestampKey: null,
    utcOffsetKey: null,
};

const TEST_DATA_DIR_URL = new URL("data/", import.meta.url);

const TEST_DATA_URLS: Readonly<Record<string, string>> = {
    "structured-cockroachdb.clp.zst":
        "https://yscope.s3.us-east-2.amazonaws.com/sample-logs/cockroachdb.clp.zst",
    "unstructured-yarn.clp.zst":
        "https://yscope.s3.us-east-2.amazonaws.com/sample-logs/yarn-ubuntu-resourcemanager-ip-172-31-17-135.log.1.clp.zst",
};

const DEFAULT_NODE_MODULE_PATH = "../build/clp-ffi-js/ClpFfiJs-node.js";

const DEFAULT_WORKER_MODULE_PATH = "../build/clp-ffi-js/ClpFfiJs-worker.js";


export {
    DEFAULT_NODE_MODULE_PATH,
    DEFAULT_READER_OPTIONS,
    DEFAULT_WORKER_MODULE_PATH,
    TEST_DATA_DIR_URL,
    TEST_DATA_URLS,
};
