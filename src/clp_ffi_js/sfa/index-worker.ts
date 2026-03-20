/**
 * SFA module entry point for worker and browser environments.
 *
 * @module
 */
import {setModule} from "./module.js";

import mainModuleFactory from "#clp-ffi-js/worker";


setModule(await mainModuleFactory());

export * from "./index.js";
