import type {
    ClpSfaReader as NativeClpSfaReader,
    MainModule,
} from "#clp-ffi-js/node";


/**
 * A high-level wrapper around the WASM-based `ClpSfaReader` module for reading CLP single-file
 * archives (SFA). This class manages the lifecycle of the underlying WASM object automatically,
 * including creation and deletion.
 *
 * Use {@link ClpArchiveReader.create} to construct an instance, and {@link ClpArchiveReader.close}
 * to release the resources.
 */
class ClpArchiveReader {
    static #modulePromise: Promise<MainModule> | null = null;

    #nativeReader: NativeClpSfaReader | null;

    /**
     * @param nativeReader The underlying WASM SfaReader instance.
     */
    private constructor (nativeReader: NativeClpSfaReader) {
        this.#nativeReader = nativeReader;
    }

    /**
     * Sets the WASM module promise used by {@link create}. Must be called exactly once before any
     * call to `create`. This is called automatically by the entry-point modules (`index-node` /
     * `index-worker`).
     *
     * @internal
     * @param modulePromise Promise resolving to the loaded WASM module.
     * @throws {Error} If `init` has already been called.
     */
    static init (modulePromise: Promise<MainModule>): void {
        if (null !== ClpArchiveReader.#modulePromise) {
            throw new Error("ClpArchiveReader.init() has already been called.");
        }
        ClpArchiveReader.#modulePromise = modulePromise;
    }

    /**
     * Creates a `ClpArchiveReader` instance from the given SFA archive data.
     *
     * @param dataArray A Uint8Array containing the SFA archive bytes.
     * @return A new ClpArchiveReader instance.
     * @throws {Error} If {@link init} has not been called or if the archive is invalid.
     */
    static async create (dataArray: Uint8Array): Promise<ClpArchiveReader> {
        if (null === ClpArchiveReader.#modulePromise) {
            throw new Error("ClpArchiveReader.init() must be called before create().");
        }
        const module = await ClpArchiveReader.#modulePromise;

        return new ClpArchiveReader(new module.ClpSfaReader(dataArray));
    }

    /**
     * Gets the number of log events in the SFA archive.
     *
     * @return The total event count as a bigint.
     * @throws {Error} If the reader has been closed.
     */
    getEventCount (): bigint {
        return this.#getNativeReader().getEventCount();
    }

    /**
     * Releases the underlying WASM resources. After calling this method, the reader is no longer
     * usable and any subsequent method calls will throw.
     *
     * This method is idempotent — calling it multiple times has no effect.
     */
    close (): void {
        if (null !== this.#nativeReader) {
            this.#nativeReader.delete();
            this.#nativeReader = null;
        }
    }

    /**
     * Returns the underlying native reader, throwing if it has been closed.
     *
     * @return The native reader instance.
     * @throws {Error} If the reader has been closed.
     */
    #getNativeReader (): NativeClpSfaReader {
        if (null === this.#nativeReader) {
            throw new Error("ClpArchiveReader has been closed.");
        }

        return this.#nativeReader;
    }
}


export {ClpArchiveReader};
