import {LogEvent} from "./LogEvent.js";
import {getModule} from "./module.js";
import type {FileInfo} from "./types.js";

import type {ClpSfaReader as WasmClpArchiveReader} from "#clp-ffi-js/node";


/**
 * A high-level wrapper around the WASM-based `ClpSfaReader` module for reading CLP single-file
 * archives (SFA). This class manages the lifecycle of the underlying WASM module and the wrapped
 * WASM object, so consumers do not need to interact with the WASM layer directly.
 *
 * Use {@link ClpArchiveReader.create} to construct an instance, and {@link ClpArchiveReader.close}
 * to release the resources.
 */
class ClpArchiveReader {
    #wasmReader: WasmClpArchiveReader | null;

    /**
     * @param wasmReader The underlying WASM SfaReader instance.
     */
    private constructor (wasmReader: WasmClpArchiveReader) {
        this.#wasmReader = wasmReader;
    }

    /**
     * Creates a `ClpArchiveReader` instance from the given SFA archive data.
     *
     * @param dataArray A Uint8Array containing the SFA archive bytes.
     * @return A promise for a new ClpArchiveReader instance.
     * @throws {Error} If the archive data cannot be parsed.
     */
    static async create (dataArray: Uint8Array): Promise<ClpArchiveReader> {
        const module = await getModule();

        return new ClpArchiveReader(new module.ClpSfaReader(dataArray));
    }

    /**
     * Gets the number of log events in the SFA archive.
     *
     * @return The total event count as a bigint.
     * @throws {Error} If the reader has been closed.
     */
    getEventCount (): bigint {
        return this.#getWasmReader().getEventCount();
    }

    /**
     * Gets source file names in range-index order.
     *
     * @return Source file names in range-index order.
     * @throws {Error} If the reader has been closed.
     */
    getFileNames (): string[] {
        return this.#getWasmReader().getFileNames();
    }

    /**
     * Gets source file metadata in range-index order.
     *
     * @return Source file metadata in range-index order.
     * @throws {Error} If the reader has been closed.
     */
    getFileInfos (): FileInfo[] {
        return this.#getWasmReader().getFileInfos();
    }

    /**
     * Decodes all log events in global log-event-index order.
     *
     * @return Decoded log events.
     * @throws {Error} If the reader has been closed.
     */
    decodeAll (): LogEvent[] {
        return (this.#getWasmReader().decodeAll() as Array<{
            logEventIdx: bigint;
            message: string;
            timestamp: bigint;
        }>).map((rawEvent) => {
            return new LogEvent(
                rawEvent.logEventIdx,
                rawEvent.timestamp,
                rawEvent.message
            );
        });
    }

    /**
     * Releases the underlying WASM resources. After calling this method, the reader is no longer
     * usable and any subsequent method calls will throw.
     *
     * This method is idempotent — calling it multiple times has no effect.
     */
    close (): void {
        if (null !== this.#wasmReader) {
            const reader = this.#wasmReader;
            this.#wasmReader = null;
            reader.delete();
        }
    }

    /**
     * Returns the underlying WASM reader, throwing if it has been closed.
     *
     * @return The WASM reader instance.
     * @throws {Error} If the reader has been closed.
     */
    #getWasmReader (): WasmClpArchiveReader {
        if (null === this.#wasmReader) {
            throw new Error("ClpArchiveReader has been closed.");
        }

        return this.#wasmReader;
    }
}


export {ClpArchiveReader};
