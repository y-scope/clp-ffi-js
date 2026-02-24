import {expect} from "vitest";

import type {
    ClpStreamReader,
    MainModule,
} from "../dist/ClpFfiJs-node.js";
import {
    DEFAULT_NODE_MODULE_PATH,
    DEFAULT_READER_OPTIONS,
    DEFAULT_WORKER_MODULE_PATH,
    TEST_DATA_DIR_URL,
} from "./constants.js";
import type {ReaderOptions} from "./types.js";


/**
 * Asserts that a value is not null or undefined, narrowing the type for later usage.
 *
 * @param val
 */
const assertNonNull: <T>(val: T) => asserts val is NonNullable<T> = (val) => {
    expect(val).not.toBeNull();
};


/**
 * Detects whether the current runtime is Node.js.
 *
 * @return True if running in Node.js.
 */
const isNodeRuntime = (): boolean => {
    // eslint-disable-next-line @typescript-eslint/no-unnecessary-condition
    return "undefined" !== typeof process && "string" === typeof process.versions?.node;
};

/**
 * Creates a new WASM module instance, using the Node.js build when running in Node.js and the
 * worker/browser build otherwise.
 *
 * @return A promise that resolves to the MainModule instance.
 */
const createModule = async (): Promise<MainModule> => {
    const modulePath =
        true === isNodeRuntime() ?

            // @ts-expect-error TS4111: property comes from index signature
            import.meta.env.VITE_NODE_MODULE_ABS_PATH ?? DEFAULT_NODE_MODULE_PATH :

            // @ts-expect-error TS4111: property comes from index signature
            import.meta.env.VITE_WORKER_MODULE_ABS_PATH ?? DEFAULT_WORKER_MODULE_PATH;
    const {default: factory} =
        // eslint-disable-next-line no-inline-comments
        await import(/* @vite-ignore */ modulePath);

    return factory();
};

/**
 * Creates a ClpStreamReader with the given options.
 *
 * @param module The WASM module instance.
 * @param data The IR stream data.
 * @param options Reader options for key extraction. Defaults to null for all keys.
 * @return A new ClpStreamReader instance.
 */
const createReader = (
    module: MainModule,
    data: Uint8Array,
    options: ReaderOptions = DEFAULT_READER_OPTIONS
): ClpStreamReader => {
    return new module.ClpStreamReader(data, options);
};

/**
 * Fetches a file and returns its bytes.
 * This function can be applied to fetching files both remotely and locally in all runtimes except
 * for local file fetch in Node.js.
 *
 * @param input File string or URL or remote URL.
 * @return File contents as a Uint8Array.
 */
const fetchFile = async (input: string | URL): Promise<Uint8Array> => {
    const response = await fetch(input);
    if (false === response.ok) {
        throw new Error(
            `Failed to fetch ${response.url}: ${response.status} ${response.statusText}`
        );
    }

    return new Uint8Array(await response.arrayBuffer());
};

/**
 * Reads a local file and returns its bytes.
 *
 * @param input File path or file URL.
 * @return File contents as a Uint8Array.
 */
const readLocalFile = async (input: string | URL): Promise<Uint8Array> => {
    if (true === isNodeRuntime()) {
        const {readFile} = await import("node:fs/promises");
        return new Uint8Array(await readFile(input));
    }

    return fetchFile(input);
};

/**
 * Loads a test data file as a Uint8Array. Test data is pre-downloaded by `globalSetup.ts`.
 * In Node.js, files are read from disk. In browser, files are served by Vite's dev server.
 *
 * @param filename The name of the file in the test data directory.
 * @return The file contents as a Uint8Array.
 */
const loadTestData = async (filename: string): Promise<Uint8Array> => {
    const fileUrl = new URL(filename, TEST_DATA_DIR_URL);

    return readLocalFile(fileUrl);
};


export type {
    ClpStreamReader,
    MainModule,
};
export {
    assertNonNull,
    createModule,
    createReader,
    fetchFile,
    loadTestData,
    readLocalFile,
};
