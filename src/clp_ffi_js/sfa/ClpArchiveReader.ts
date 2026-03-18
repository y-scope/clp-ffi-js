import {LogEvent} from "./LogEvent.js";
import type {FileInfoArray} from "./types.js";
import {isNodeRuntime} from "../utils.js";
import NodeModuleFactory from "#clp-ffi-js/node";
import WorkerModuleFactory from "#clp-ffi-js/worker";

import type {
    ClpSfaReader,
    MainModule,
} from "#clp-ffi-js/node";


/**
 * JavaScript FFI binding for the C++ SFA archive reader:
 * [`clp_s::ffi::sfa::ClpArchiveReader`](
 *     ../../../build/deps/clp/components/core/src/clp_s/ffi/sfa/ClpArchiveReader.hpp
 * ).
 */
class ClpArchiveReader {
    static #modulePromise: Promise<MainModule> | null = null;

    readonly #native: ClpSfaReader;

    #closed: boolean = false;

    private constructor (native: ClpSfaReader) {
        this.#native = native;
    }

    /**
     * Creates a reader from in-memory archive bytes.
     * Lazily initializes and caches the underlying module instance on first use.
     *
     * @param archiveData Single-file archive bytes.
     * @return Reader instance.
     */
    static async create (archiveData: Uint8Array): Promise<ClpArchiveReader> {
        const module = await ClpArchiveReader.#init();
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
    decodeAll (): LogEvent[] {
        this.#assertOpen();

        return (this.#native.decodeAll() as Array<{
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

    /**
     * Initializes and caches the WASM module promise.
     *
     * @return The cached module promise.
     */
    static #init (): Promise<MainModule> {
        if (null === ClpArchiveReader.#modulePromise) {
            const moduleFactory = (
                true === isNodeRuntime() ?
                    NodeModuleFactory :
                    WorkerModuleFactory
            ) as () => Promise<MainModule>;
            ClpArchiveReader.#modulePromise = moduleFactory();
        }
        return ClpArchiveReader.#modulePromise;
    }
}

export {ClpArchiveReader};
