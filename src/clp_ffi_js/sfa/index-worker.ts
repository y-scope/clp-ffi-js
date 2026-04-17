/**
 * SFA module entry point for worker and browser environments.
 *
 * @module
 */
import {setModuleFactory} from "./module.js";

import mainModuleFactory from "#clp-ffi-js/worker";
import workerWasmUrl from "../../ClpFfiJs-worker.wasm?url";


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
