import type {MainModule} from "#clp-ffi-js/node";


let mainModule: MainModule | null = null;

/**
 * Sets the shared WASM module.
 *
 * @param newMainModule The loaded WASM module.
 * @throws {Error} If the module has already been set.
 */
const setModule = (newMainModule: MainModule): void => {
    if (null !== mainModule) {
        throw new Error("WASM module has already been initialized.");
    }
    mainModule = newMainModule;
};

/**
 * Returns the shared WASM module.
 *
 * @return The loaded module.
 * @throws {Error} If module has not been set.
 */
const getModule = (): MainModule => {
    if (null === mainModule) {
        throw new Error("WASM module has not been initialized.");
    }

    return mainModule;
};

export {
    getModule,
    setModule,
};
