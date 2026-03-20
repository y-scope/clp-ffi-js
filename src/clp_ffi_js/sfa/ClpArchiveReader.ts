import {getModule} from "./module.js";

import type {ClpSfaReader} from "#clp-ffi-js/node";


/**
 * A high-level wrapper around the WASM-based `ClpSfaReader` module for reading CLP single-file
 * archives (SFA). This class manages the lifecycle of the underlying WASM object automatically,
 * including creation and deletion.
 *
 * Use {@link ClpArchiveReader.create} to construct an instance, and {@link ClpArchiveReader.close}
 * to release the resources.
 */
class ClpArchiveReader {
    #nativeReader: ClpSfaReader | null;

    /**
     * @param nativeReader The underlying WASM SfaReader instance.
     */
    private constructor (nativeReader: ClpSfaReader) {
        this.#nativeReader = nativeReader;
    }

    /**
     * Creates a `ClpArchiveReader` instance from the given SFA archive data.
     *
     * @param dataArray A Uint8Array containing the SFA archive bytes.
     * @return A new ClpArchiveReader instance.
     * @throws {Error} If the WASM module has not been initialized or if the archive is invalid.
     * @throws {TypeError} If `dataArray` is not a Uint8Array.
     */
    static create (dataArray: Uint8Array): ClpArchiveReader {
        if (false === (dataArray instanceof Uint8Array)) {
            throw new TypeError("dataArray must be a Uint8Array.");
        }
        const module = getModule();

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
    #getNativeReader (): ClpSfaReader {
        if (null === this.#nativeReader) {
            throw new Error("ClpArchiveReader has been closed.");
        }

        return this.#nativeReader;
    }
}


export {ClpArchiveReader};
