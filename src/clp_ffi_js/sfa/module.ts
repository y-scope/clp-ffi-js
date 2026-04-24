import type {MainModule} from "#clp-ffi-js/node";


let mainModuleFactory: (() => Promise<MainModule>) | null = null;
let mainModulePromise: Promise<MainModule> | null = null;

/**
 * Sets the shared WASM module factory.
 *
 * @param newMainModuleFactory Factory for loading the WASM module.
 * @throws {Error} If the factory has already been set.
 */
const setModuleFactory = (newMainModuleFactory: () => Promise<MainModule>): void => {
    if (null !== mainModuleFactory) {
        throw new Error("WASM module factory has already been set.");
    }
    mainModuleFactory = newMainModuleFactory;
};

/**
 * Returns the shared WASM module.
 *
 * @return A promise that resolves to the loaded module.
 * @throws {Error} If the module factory has not been set via {@link setModuleFactory}.
 * @throws {Error} Propagates `mainModuleFactory`'s exceptions.
 */
const getModule = async (): Promise<MainModule> => {
    if (null !== mainModulePromise) {
        return mainModulePromise;
    }
    if (null === mainModuleFactory) {
        throw new Error("WASM module factory has not been set.");
    }

    const promise = mainModuleFactory();

    // Clear the cached promise on rejection so the next caller retries.
    promise.catch(() => {
        if (mainModulePromise === promise) {
            mainModulePromise = null;
        }
    });
    mainModulePromise = promise;

    return mainModulePromise;
};

export {
    getModule,
    setModuleFactory,
};
