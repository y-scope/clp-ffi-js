import {LogEvent} from "./LogEvent.js";
import {getModule} from "./module.js";
import type {FileInfo} from "./types.js";

import type {ClpSfaReader as WasmClpArchiveReader} from "#clp-ffi-js/node";


interface RawLogEvent {
    logEventIdx: bigint;
    message: string;
    timestamp: bigint;
}

/**
 * Converts log events returned by the WASM binding into the public JavaScript representation.
 *
 * @param rawEvents Log events returned by the WASM binding.
 * @return Public log-event objects.
 */
const createLogEvents = (rawEvents: RawLogEvent[]): LogEvent[] => rawEvents.map((rawEvent) => {
    return new LogEvent(
        rawEvent.logEventIdx,
        rawEvent.timestamp,
        rawEvent.message
    );
});


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
     * @throws {Error} If the archive data cannot be loaded or parsed.
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
     * Gets the number of log events in the selected source file, or the total archive event count
     * when no source file is selected.
     *
     * @return The active event count as a bigint.
     * @throws {Error} If the reader has been closed.
     */
    getActiveEventCount (): bigint {
        return this.#getWasmReader().getActiveEventCount();
    }

    /**
     * Gets the total size of the original uncompressed logs represented by the archive.
     *
     * @return The uncompressed size in bytes.
     * @throws {Error} If the reader has been closed.
     */
    getUncompressedSize (): bigint {
        return this.#getWasmReader().getUncompressedSize();
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
     * Finds source file metadata by exact, case-sensitive filename.
     *
     * @param fileName Source filename stored in the archive range index.
     * @return The source file metadata, or `null` if the filename doesn't exist.
     * @throws {Error} If the reader has been closed.
     */
    getFileInfo (fileName: string): FileInfo | null {
        return this.#getWasmReader().getFileInfo(fileName);
    }

    /**
     * Gets the selected source filename.
     *
     * @return The selected filename, or `null` when the entire archive is active.
     * @throws {Error} If the reader has been closed.
     */
    getSelectedFileName (): string | null {
        return this.#getWasmReader().getSelectedFileName();
    }

    /**
     * Selects a source file as the active log-event collection.
     *
     * This method must be called before decoding or searching begins.
     *
     * @param fileName Source filename stored in the archive range index.
     * @throws {Error} If the reader is closed, the filename doesn't exist, or decoding has begun.
     */
    selectFile (fileName: string): void {
        this.#getWasmReader().selectFile(fileName);
    }

    /**
     * Gets the mapping from filtered positions to unfiltered decoded-event indices.
     *
     * @return The filtered-event map, or `null` when no filter is active or all events match.
     */
    getFilteredLogEventMap (): number[] | null {
        return this.#getWasmReader().getFilteredLogEventMap();
    }

    /**
     * Replaces the current KQL filter.
     *
     * The user query is evaluated case-sensitively. The generated log-level query is evaluated
     * case-insensitively, and the two result sets are intersected.
     *
     * @param kqlFilter User-provided KQL query.
     * @param logLevelKqlFilter Generated KQL query for log-level filtering.
     */
    filterLogEvents (kqlFilter: string, logLevelKqlFilter = ""): void {
        this.#getWasmReader().filterLogEvents(kqlFilter, logLevelKqlFilter);
    }

    /**
     * Clears the current KQL query and filtered-event map.
     */
    clearQuery (): void {
        this.#getWasmReader().clearQuery();
    }

    /**
     * Decodes and caches all log events without returning them.
     *
     * @throws {Error} If the reader has been closed or decoding fails.
     */
    decode (): void {
        this.#getWasmReader().decode();
    }

    /**
     * Decodes all log events in the active collection. Log-event indices are relative to the
     * selected source file when one is selected.
     *
     * @return Decoded log events.
     * @throws {Error} If the reader has been closed.
     */
    decodeAll (): LogEvent[] {
        return createLogEvents(this.#getWasmReader().decodeAll() as RawLogEvent[]);
    }

    /**
     * Decodes all log events in the active collection, if necessary, and returns the requested
     * half-open event range. Indices are relative to the selected source file when one is selected.
     *
     * @param beginIdx Index of the first event to return.
     * @param endIdx Index one past the final event to return.
     * @param useFilter Whether to decode from the filtered event collection.
     * @return Decoded log events, or `null` if the requested collection or range is unavailable.
     * @throws {Error} If the reader has been closed, decoding fails, or the range is invalid.
     */
    decodeRange (beginIdx: number, endIdx: number): LogEvent[];

    decodeRange (beginIdx: number, endIdx: number, useFilter: boolean): LogEvent[] | null;

    decodeRange (beginIdx: number, endIdx: number, useFilter = false): LogEvent[] | null {
        const rawEvents = this.#getWasmReader().decodeRange(beginIdx, endIdx, useFilter) as
            RawLogEvent[] | null;

        return null === rawEvents ?
            null :
            createLogEvents(rawEvents);
    }

    /**
     * Finds the last log event whose timestamp is less than or equal to `targetTimestamp`.
     *
     * If `targetTimestamp` precedes every event, returns the first event's index.
     *
     * @param targetTimestamp Epoch timestamp in milliseconds.
     * @return The zero-based log-event index, or `null` if the archive contains no log events.
     * @throws {Error} If the reader has been closed or decoding fails.
     */
    findNearestLogEventByTimestamp (targetTimestamp: bigint): number | null {
        return this.#getWasmReader().findNearestLogEventByTimestamp(targetTimestamp);
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
