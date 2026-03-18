/**
 * SFA module entry point for Node.js environments.
 *
 * @module
 */
import {ClpArchiveReader} from "./ClpArchiveReader.js";

import MainModuleFactory from "#clp-ffi-js/node";


// eslint-disable-next-line new-cap
ClpArchiveReader.init(MainModuleFactory());

export * from "./index.js";
