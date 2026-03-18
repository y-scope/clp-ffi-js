/**
 * SFA module entry point for worker and browser environments.
 *
 * @module
 */
import {ClpArchiveReader} from "./ClpArchiveReader.js";

import MainModuleFactory from "#clp-ffi-js/worker";


// eslint-disable-next-line new-cap
ClpArchiveReader.init(MainModuleFactory());

export * from "./index.js";
