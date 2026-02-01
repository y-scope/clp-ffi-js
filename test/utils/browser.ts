import MainModuleFactory, {type MainModule} from "../../dist/ClpFfiJs-worker.js";
import {TEST_DATA_URLS} from "./common.js";


/**
 * Loads a test data file as a Uint8Array by fetching from S3.
 *
 * @param filename The name of the test data file.
 * @return The file contents as a Uint8Array.
 */
const loadTestData = async (filename: string): Promise<Uint8Array> => {
    const url = TEST_DATA_URLS[filename];
    if ("undefined" === typeof url) {
        throw new Error(`Unknown test data file: ${filename}`);
    }

    const response = await fetch(url);
    return new Uint8Array(await response.arrayBuffer());
};

/**
 * Creates a new WASM module instance (worker/browser build).
 *
 * @return A promise that resolves to the MainModule instance.
 */
const createModule = async (): Promise<MainModule> => {
    // eslint-disable-next-line new-cap
    return MainModuleFactory();
};


export {
    createModule,
    loadTestData,
};
