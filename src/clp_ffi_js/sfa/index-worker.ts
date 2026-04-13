/**
 * SFA module entry point for worker and browser environments.
 *
 * NOTE: This entry point uses the `?url` import suffix to resolve the `.wasm` asset, which is a
 * Vite/Rollup-specific convention and is not recognized by every bundler (e.g., Webpack, esbuild).
 * Consumers using other bundlers may need to provide their own entry point.
 * @module
 */
import {setModuleFactory} from "./module.js";

import mainModuleFactory from "#clp-ffi-js/worker";
import workerWasmUrl from "#clp-ffi-js/worker-wasm?url";


// Emscripten's default `locateFile` resolves the `.wasm` file against the current page URL, which
// fails when a bundler emits the `.wasm` under a hashed asset path separate from the JS bundle.
// Override it to return the bundler-resolved URL instead.
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
