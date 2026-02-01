import fs from "node:fs";
import path from "node:path";
import {pipeline} from "node:stream/promises";
import {fileURLToPath} from "node:url";

import axios from "axios";

import MainModuleFactory, {type MainModule} from "../../dist/ClpFfiJs-node.js";
import {TEST_DATA_URLS} from "./common.js";


const TEST_DATA_DIR = path.join(path.dirname(fileURLToPath(import.meta.url)), "..", "data");

/**
 * Downloads a file from a URL to a local path if it doesn't already exist.
 *
 * @param url The URL to download from.
 * @param filePath The local file path to save to.
 */
const downloadFileIfNeeded = async (url: string, filePath: string): Promise<void> => {
    if (fs.existsSync(filePath)) {
        return;
    }

    fs.mkdirSync(path.dirname(filePath), {recursive: true});

    const response = await axios.get(url, {responseType: "stream"});
    await pipeline(response.data, fs.createWriteStream(filePath));
};

/**
 * Loads a test data file as a Uint8Array, downloading it from S3 if it doesn't exist locally.
 *
 * @param filename The name of the file in the test data directory.
 * @return The file contents as a Uint8Array.
 */
const loadTestData = async (filename: string): Promise<Uint8Array> => {
    const filePath = path.join(TEST_DATA_DIR, filename);

    const url = TEST_DATA_URLS[filename];
    if ("undefined" === typeof url) {
        throw new Error(`Unknown test data file: ${filename}`);
    }

    await downloadFileIfNeeded(url, filePath);

    return new Uint8Array(fs.readFileSync(filePath));
};

/**
 * Creates a new WASM module instance (Node.js build).
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
