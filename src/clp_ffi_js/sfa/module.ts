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
        throw new Error("WASM module factory has already been initialized.");
    }
    mainModuleFactory = newMainModuleFactory;
};

/**
 * Returns the shared WASM module.
 *
 * @return The loaded module.
 * @throws {Error} If the module factory has not been set via {@link setModuleFactory}.
 */
const getModule = async (): Promise<MainModule> => {
    if (null === mainModulePromise) {
        if (null === mainModuleFactory) {
            throw new Error("WASM module factory has not been initialized.");
        }

        mainModulePromise = mainModuleFactory();
    }

    return mainModulePromise;
};

export {
    getModule,
    setModuleFactory,
};
