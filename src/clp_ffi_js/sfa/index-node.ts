/**
 * SFA module entry point for Node.js environments.
 *
 * NOTE: This module uses top-level `await` to initialize the WASM module at import time. If WASM
 * loading fails, the module itself will fail to load. To handle such errors, use a dynamic
 * `import()` wrapped in a try/catch rather than a static `import` statement.
 *
 * @module
 */
import {setModule} from "./module.js";

import mainModuleFactory from "#clp-ffi-js/node";


setModule(await mainModuleFactory());

export * from "./index.js";
