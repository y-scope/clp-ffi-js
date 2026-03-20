/**
 * SFA module entry point for Node.js environments.
 *
 * @module
 */
import {setModule} from "./module.js";

import mainModuleFactory from "#clp-ffi-js/node";


setModule(await mainModuleFactory());

export * from "./index.js";
