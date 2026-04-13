import type {MainModule} from "clp-ffi-js/node";
import {
    describe,
    expect,
    it,
    vi,
} from "vitest";


type ModuleApi = typeof import("../src/clp_ffi_js/sfa/module.js");

let freshModuleCounter = 0;

/**
 * Imports `module.ts` with fresh singleton state. Uses a cache-busting query string so each call
 * yields a distinct module instance; this works across Node and browser, whereas
 * `vi.resetModules()` alone does not reliably reset module-level state in the browser.
 *
 * @return A fresh copy of the module's exports.
 */
const loadFreshModule = async (): Promise<ModuleApi> => {
    freshModuleCounter += 1;
    const specifier = `../src/clp_ffi_js/sfa/module.js?v=${freshModuleCounter.toString()}`;

    // eslint-disable-next-line no-inline-comments
    return (await import(/* @vite-ignore */ specifier)) as ModuleApi;
};

/**
 * Creates a deferred promise for testing single-flight behavior.
 *
 * @return Promise and its resolve/reject callbacks.
 */
const createDeferred = <T>(): {
    promise: Promise<T>;
    reject: (reason?: unknown) => void;
    resolve: (value: T | PromiseLike<T>) => void;
} => {
    let resolve!: (value: T | PromiseLike<T>) => void;
    let reject!: (reason?: unknown) => void;
    const promise = new Promise<T>((innerResolve, innerReject) => {
        resolve = innerResolve;
        reject = innerReject;
    });

    return {promise, reject, resolve};
};

describe("sfa/module.ts", () => {
    describe("getModule", () => {
        it("throws if the factory has not been set", async () => {
            const {getModule} = await loadFreshModule();

            await expect(getModule()).rejects.toThrow();
        });

        it("invokes the factory exactly once across concurrent callers", async () => {
            const {setModuleFactory, getModule} = await loadFreshModule();

            const deferred = createDeferred<MainModule>();
            const stubModule = {} as MainModule;
            const factory = vi.fn(() => deferred.promise);
            setModuleFactory(factory);

            const pendingResults = [
                getModule(),
                getModule(),
                getModule(),
            ];

            expect(factory).toHaveBeenCalledTimes(1);

            deferred.resolve(stubModule);
            const [result1,
                result2,
                result3] = await Promise.all(pendingResults);

            expect(result1).toBe(stubModule);
            expect(result2).toBe(stubModule);
            expect(result3).toBe(stubModule);
        });

        it("returns the same cached module on subsequent calls after resolution", async () => {
            const {setModuleFactory, getModule} = await loadFreshModule();

            const stubModule = {} as MainModule;
            const factory = vi.fn(() => Promise.resolve(stubModule));
            setModuleFactory(factory);

            await expect(getModule()).resolves.toBe(stubModule);
            await expect(getModule()).resolves.toBe(stubModule);
            await expect(getModule()).resolves.toBe(stubModule);
            expect(factory).toHaveBeenCalledTimes(1);
        });

        it("clears the cached promise on rejection so the next caller retries", async () => {
            const {setModuleFactory, getModule} = await loadFreshModule();

            const stubModule = {} as MainModule;
            const factory = vi.fn<() => Promise<MainModule>>()
                .mockRejectedValueOnce(new Error("transient failure"))
                .mockResolvedValueOnce(stubModule);

            setModuleFactory(factory);

            await expect(getModule()).rejects.toThrow();
            await Promise.resolve();
            await expect(getModule()).resolves.toBe(stubModule);
            expect(factory).toHaveBeenCalledTimes(2);
        });
    });

    describe("setModuleFactory", () => {
        it("throws if called more than once", async () => {
            const {setModuleFactory} = await loadFreshModule();

            const noopFactory = (): Promise<MainModule> => Promise.resolve({} as MainModule);
            setModuleFactory(noopFactory);

            expect(() => {
                setModuleFactory(noopFactory);
            }).toThrow();
        });
    });
});
