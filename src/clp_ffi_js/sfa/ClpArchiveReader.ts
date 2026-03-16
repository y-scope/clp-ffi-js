import type {
    ClpSfaReader,
    MainModule,
} from "../../../dist/ClpFfiJs-node.js";
import {LogEvent} from "./LogEvent.js";
import type {FileInfoArray} from "./types.js";


/**
 * Type-safe wrapper around the native `ClpSfaReader` binding.
 */
class ClpArchiveReader {
    readonly #native: ClpSfaReader;

    #closed: boolean = false;

    private constructor (native: ClpSfaReader) {
        this.#native = native;
    }

    /**
     * Creates a reader from in-memory archive bytes.
     *
     * @param module Loaded WASM module.
     * @param archiveData Single-file archive bytes.
     * @return Reader instance.
     */
    static create (module: MainModule, archiveData: Uint8Array): ClpArchiveReader {
        return new ClpArchiveReader(new module.ClpSfaReader(archiveData));
    }

    /**
     * Releases native resources.
     */
    close (): void {
        if (this.#closed) {
            return;
        }

        this.#closed = true;
        this.#native.delete();
    }

    /**
     * Gets the total number of events in the archive.
     *
     * @return Total number of events in the archive.
     */
    getEventCount (): bigint {
        this.#assertOpen();

        return this.#native.getEventCount();
    }

    /**
     * Gets source file names in range-index order.
     *
     * @return Source file names in range-index order.
     */
    getFileNames (): string[] {
        this.#assertOpen();

        return this.#native.getFileNames();
    }

    /**
     * Gets source file metadata in range-index order.
     *
     * @return Source file metadata in range-index order.
     */
    getFileInfos (): FileInfoArray {
        this.#assertOpen();

        return this.#native.getFileInfos() as FileInfoArray;
    }

    /**
     * Decodes all log events in global log-event-index order.
     *
     * @return Decoded log events.
     */
    decode (): LogEvent[] {
        this.#assertOpen();

        return (this.#native.decode() as Array<{
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

    #assertOpen (): void {
        if (this.#closed) {
            throw new Error("ClpArchiveReader is closed.");
        }
    }
}

export {ClpArchiveReader};
