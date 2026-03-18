/**
 * Detects whether the current runtime is Node.js.
 *
 * @return True if running in Node.js.
 * @internal
 */
const isNodeRuntime = (): boolean => {
    const maybeWindow = (globalThis as {window?: unknown}).window;
    if (undefined !== maybeWindow) {
        return false;
    }

    const maybeProcess = (globalThis as {process?: {versions?: {node?: unknown}}}).process;
    return "string" === typeof maybeProcess?.versions?.node;
};

export {isNodeRuntime};
