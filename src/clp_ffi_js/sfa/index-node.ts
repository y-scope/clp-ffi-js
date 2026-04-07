/**
 * SFA module entry point for Node.js environments.
 *
 * @module
 */
import {setModuleFactory} from "./module.js";

import mainModuleFactory from "#clp-ffi-js/node";


setModuleFactory(mainModuleFactory);

export * from "./index.js";
