import {
    afterEach,
    beforeAll,
    beforeEach,
    describe,
    expect,
    it,
} from "vitest";

import {
    DECODE_CHUNK_SIZE,
    FILTERED_CHUNK_SIZE,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    OUT_OF_BOUNDS_OFFSET,
} from "./constants.js";
import {
    assertNonNull,
    type ClpStreamReader,
    createReader,
    type MainModule,
    type ReaderOptions,
} from "./utils/common.js";
import {
    createModule,
    loadTestData,
} from "./utils/index.js";


let module: MainModule;
let data: Uint8Array;

beforeAll(async () => {
    module = await createModule();
    data = await loadTestData("cockroachdb.clp.zst");
});

// eslint-disable-next-line max-lines-per-function
describe("Structured IR Stream: cockroachdb.clp.zst", () => {
    let reader: ClpStreamReader;

    beforeEach(() => {
        reader = createReader(module, data);
    });

    afterEach(() => {
        reader.delete();
    });

    it("should create a ClpStreamReader", () => {
        expect(reader).not.toBeNull();
    });

    it("should report IR stream type as STRUCTURED", () => {
        const streamType = reader.getIrStreamType();

        expect(streamType.value).toBe(module.IrStreamType.STRUCTURED.value);
    });

    it("should return metadata as an object", () => {
        const metadata = reader.getMetadata();

        expect(metadata).not.toBeNull();
        expect(typeof metadata).toBe("object");
    });

    it("should deserialize stream and return a positive event count", () => {
        const numEvents = reader.deserializeStream();

        expect(numEvents).toBeGreaterThan(0);
    });

    it("should match getNumEventsBuffered with deserialized count", () => {
        const numEvents = reader.deserializeStream();
        const numBuffered = reader.getNumEventsBuffered();

        expect(numBuffered).toBe(numEvents);
    });

    it("should decode first events with expected fields", () => {
        const numEvents = reader.deserializeStream();

        const events = reader.decodeRange(0, Math.min(DECODE_CHUNK_SIZE, numEvents), false);

        assertNonNull(events);
        expect(events).toHaveLength(Math.min(DECODE_CHUNK_SIZE, numEvents));

        const [firstEvent] = events;
        assertNonNull(firstEvent);

        expect(typeof firstEvent.logEventNum).toBe("number");
        expect(typeof firstEvent.logLevel).toBe("number");
        expect(typeof firstEvent.message).toBe("string");
        expect(typeof firstEvent.timestamp).toBe("bigint");
        expect([
            "bigint",
            "number",
        ]).toContain(typeof firstEvent.utcOffset);
    });

    it("should decode last events", () => {
        const numEvents = reader.deserializeStream();

        const lastEvents = reader.decodeRange(
            Math.max(0, numEvents - FILTERED_CHUNK_SIZE),
            numEvents,
            false
        );

        assertNonNull(lastEvents);
        expect(lastEvents.length).toBeGreaterThan(0);
    });

    it("should filter log events by log level", () => {
        const numEvents = reader.deserializeStream();

        reader.filterLogEvents([LOG_LEVEL_ERROR]);
        const errorMap = reader.getFilteredLogEventMap();

        assertNonNull(errorMap);
        expect(errorMap.length).toBeLessThanOrEqual(numEvents);
    });

    it("should reset filter when passing null", () => {
        reader.deserializeStream();

        reader.filterLogEvents([LOG_LEVEL_ERROR]);
        reader.filterLogEvents(null);
        const resetMap = reader.getFilteredLogEventMap();

        expect(resetMap).toBeNull();
    });

    it("should decode events with useFilter=true", () => {
        reader.deserializeStream();

        reader.filterLogEvents([LOG_LEVEL_INFO]);
        const filteredMap = reader.getFilteredLogEventMap();

        assertNonNull(filteredMap);
        if (0 < filteredMap.length) {
            const filteredEvents = reader.decodeRange(
                0,
                Math.min(FILTERED_CHUNK_SIZE, filteredMap.length),
                true
            );

            assertNonNull(filteredEvents);
            expect(filteredEvents.length).toBeGreaterThan(0);
        }
    });

    it("should filter log events with KQL query", () => {
        reader.deserializeStream();

        reader.filterLogEvents(null, "loglevel: INFO");
        const kqlMap = reader.getFilteredLogEventMap();

        assertNonNull(kqlMap);
        expect(kqlMap.length).toBeGreaterThanOrEqual(0);

        if (0 < kqlMap.length) {
            const kqlEvents = reader.decodeRange(
                0,
                Math.min(FILTERED_CHUNK_SIZE, kqlMap.length),
                true
            );

            assertNonNull(kqlEvents);
        }
    });

    it("should filter log events with KQL + log level combined", () => {
        reader.deserializeStream();

        reader.filterLogEvents([
            LOG_LEVEL_INFO,
            LOG_LEVEL_WARN,
            LOG_LEVEL_ERROR,
        ], "server");
        const combinedMap = reader.getFilteredLogEventMap();

        assertNonNull(combinedMap);
        expect(combinedMap.length).toBeGreaterThanOrEqual(0);
    });

    it("should find nearest log event by timestamp", () => {
        const numEvents = reader.deserializeStream();

        const events = reader.decodeRange(0, Math.min(DECODE_CHUNK_SIZE, numEvents), false);
        assertNonNull(events);

        const [firstEvent] = events;
        assertNonNull(firstEvent);
        const nearestIdx = reader.findNearestLogEventByTimestamp(firstEvent.timestamp);

        expect(nearestIdx).not.toBeNull();
        expect(typeof nearestIdx).toBe("number");
    });

    it("should return null for out-of-bounds decodeRange", () => {
        const numEvents = reader.deserializeStream();

        const invalidRange = reader.decodeRange(
            numEvents + OUT_OF_BOUNDS_OFFSET,
            numEvents + (2 * OUT_OF_BOUNDS_OFFSET),
            false
        );

        expect(invalidRange).toBeNull();
    });
});

describe("Structured IR Stream with logLevelKey", () => {
    const readerOptions: ReaderOptions = {
        logLevelKey: {isAutoGenerated: true, parts: ["loglevel"]},
        timestampKey: null,
        utcOffsetKey: null,
    };

    let reader: ClpStreamReader;

    beforeEach(() => {
        reader = createReader(module, data, readerOptions);
    });

    afterEach(() => {
        reader.delete();
    });

    it("should extract log levels from events", () => {
        const numEvents = reader.deserializeStream();

        const events = reader.decodeRange(
            0,
            Math.min(DECODE_CHUNK_SIZE, numEvents),
            false
        );

        assertNonNull(events);
        const [firstEvent] = events;
        assertNonNull(firstEvent);

        expect(firstEvent.logLevel).not.toBe(0);
    });

    it("should filter INFO events with extracted log levels", () => {
        const numEvents = reader.deserializeStream();

        reader.filterLogEvents([LOG_LEVEL_INFO]);
        const infoMap = reader.getFilteredLogEventMap();

        assertNonNull(infoMap);
        expect(infoMap.length).toBeGreaterThan(0);
        expect(infoMap.length).toBeLessThan(numEvents);
    });

    it("should decode filtered events with useFilter=true", () => {
        reader.deserializeStream();

        reader.filterLogEvents([LOG_LEVEL_INFO]);
        const filteredMap = reader.getFilteredLogEventMap();
        assertNonNull(filteredMap);

        const filteredEvents = reader.decodeRange(
            0,
            Math.min(FILTERED_CHUNK_SIZE, filteredMap.length),
            true
        );

        assertNonNull(filteredEvents);
        expect(filteredEvents.length).toBeGreaterThan(0);
    });

    it("should combine KQL and log level filters with extracted keys", () => {
        reader.deserializeStream();

        reader.filterLogEvents([
            LOG_LEVEL_INFO,
            LOG_LEVEL_WARN,
            LOG_LEVEL_ERROR,
        ], "server");
        const combinedMap = reader.getFilteredLogEventMap();

        assertNonNull(combinedMap);
        expect(combinedMap.length).toBeGreaterThanOrEqual(0);
    });
});
