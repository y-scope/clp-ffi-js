import NodeModuleFactory, {type MainModule} from "#clp-ffi-js/node";
import WorkerModuleFactory from "#clp-ffi-js/worker";


/**
 * Detects whether the current runtime is Node.js.
 *
 * @return True if running in Node.js.
 */
const isNodeRuntime = (): boolean => {
    const maybeWindow = (globalThis as {window?: unknown}).window;
    if ("undefined" !== typeof maybeWindow) {
        return false;
    }

    const maybeProcess = (globalThis as {process?: {versions?: {node?: unknown}}}).process;
    return "string" === typeof maybeProcess?.versions?.node;
};

let mainModulePromise: Promise<MainModule> | null = null;

/**
 * Returns the shared CLP FFI module instance, initializing it lazily once.
 *
 * @return A cached `MainModule` promise.
 */
const getMainModule = (): Promise<MainModule> => {
    if (null !== mainModulePromise) {
        return mainModulePromise;
    }

    const moduleFactory = (
        true === isNodeRuntime() ?
            NodeModuleFactory :
            WorkerModuleFactory
    ) as () => Promise<MainModule>;

    mainModulePromise = moduleFactory();

    return mainModulePromise;
};

export {getMainModule};
