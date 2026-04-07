/**
 * SFA module entry point for worker and browser environments.
 *
 * @module
 */
import {setModuleFactory} from "./module.js";

import mainModuleFactory from "#clp-ffi-js/worker";

setModuleFactory(mainModuleFactory);

export * from "./index.js";
