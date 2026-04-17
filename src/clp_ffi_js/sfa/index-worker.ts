/**
 * SFA module entry point for worker and browser environments.
 *
 * Emscripten uses `locateFile` to resolve runtime assets such as the `.wasm`
 * binary. The function receives the relative path to the file as configured in
 * build process plus a prefix derived from main JavaScript file's directory.
 * It should return the full URL where the file can be loaded.
 *
 * Currently, the resolution code inside `ClpFfiJs-worker.js` is:
 *
 *   l.locateFile
 *     ? l.locateFile("ClpFfiJs-worker.wasm", prefix)
 *     : (new URL("ClpFfiJs-worker.wasm", import.meta.url)).href
 *
 * We override `locateFile` and provide the exact `.wasm` URL ahead of time so
 * resolution does not depend on fragile runtime path inference in downstream
 * bundlers. For example, a bundler may emit the `.wasm` file at a hashed asset
 * URL separate from the JavaScript bundle that contains the Emscripten glue,
 * causing the default relative-path resolution to fail.
 *
 * In Vite/Rollup, the `?url` suffix can be applied to asset imports to obtain
 * the final emitted URL directly. This syntax is not universally supported by
 * all bundlers (e.g., Webpack, esbuild). Separate bundler-specific entry points
 * can be provided if needed.
 *
 * The `?url` transform must be applied to a relative path because downstream
 * Vite dependency optimization does not reliably handle `?url` on package
 * import specifiers. So we cannot write something like:
 *
 *   import workerWasmUrl from "#clp-ffi-js/worker-wasm?url";
 *
 * See also:
 * https://emscripten.org/docs/api_reference/module.html#Module.locateFile
 * https://vite.dev/guide/features.html#accessing-the-webassembly-module
 *
 * @module
 */
// eslint-disable-next-line import/no-unresolved
import workerWasmUrl from "../../ClpFfiJs-worker.wasm?url";
import {setModuleFactory} from "./module.js";

import mainModuleFactory from "#clp-ffi-js/worker";


setModuleFactory(() => {
    return mainModuleFactory({
        locateFile: (path: string) => {
            if ("ClpFfiJs-worker.wasm" === path) {
                return workerWasmUrl;
            }

            return path;
        },
    });
});

export * from "./index.js";
