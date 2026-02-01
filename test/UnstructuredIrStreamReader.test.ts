import {
    afterEach,
    beforeAll,
    beforeEach,
    describe,
    expect,
    it,
} from "vitest";

import {
    type ClpStreamReader,
    createModule,
    createReader,
    DECODE_CHUNK_SIZE,
    FILTERED_CHUNK_SIZE,
    loadTestData,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    type MainModule,
    OUT_OF_BOUNDS_OFFSET,
} from "./utils.js";


let module: MainModule;
let data: Uint8Array;

beforeAll(async () => {
    module = await createModule();
    data = await loadTestData("yarn-unstructured.clp.zst");
});

// eslint-disable-next-line max-lines-per-function
describe("Unstructured IR Stream: yarn-unstructured.clp.zst", () => {
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

    it("should report IR stream type as UNSTRUCTURED", () => {
        const streamType = reader.getIrStreamType();

        expect(streamType.value).toBe(module.IrStreamType.UNSTRUCTURED.value);
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

        expect(events).not.toBeNull();
        expect(events).toHaveLength(Math.min(DECODE_CHUNK_SIZE, numEvents));

        const [event] = events as NonNullable<typeof events>;

        expect(typeof event.logEventNum).toBe("number");
        expect(typeof event.logLevel).toBe("number");
        expect(typeof event.message).toBe("string");
        expect(typeof event.timestamp).toBe("bigint");
        expect([
            "bigint",
            "number",
        ]).toContain(typeof event.utcOffset);
    });

    it("should decode last events", () => {
        const numEvents = reader.deserializeStream();

        const lastEvents = reader.decodeRange(
            Math.max(0, numEvents - FILTERED_CHUNK_SIZE),
            numEvents,
            false
        );

        expect(lastEvents).not.toBeNull();
        expect((lastEvents as NonNullable<typeof lastEvents>).length).toBeGreaterThan(0);
    });

    it("should filter log events by log level", () => {
        const numEvents = reader.deserializeStream();

        reader.filterLogEvents([LOG_LEVEL_INFO]);
        const infoMap = reader.getFilteredLogEventMap();

        if (null !== infoMap) {
            expect(infoMap.length).toBeLessThanOrEqual(numEvents);
        }
    });

    it("should reset filter when passing null", () => {
        reader.deserializeStream();

        reader.filterLogEvents([LOG_LEVEL_INFO]);
        reader.filterLogEvents(null);
        const resetMap = reader.getFilteredLogEventMap();

        expect(resetMap).toBeNull();
    });

    it("should decode events with useFilter=true", () => {
        reader.deserializeStream();

        reader.filterLogEvents([
            LOG_LEVEL_WARN,
            LOG_LEVEL_ERROR,
        ]);
        const warnErrorMap = reader.getFilteredLogEventMap();

        if (null !== warnErrorMap && 0 < warnErrorMap.length) {
            const filteredEvents = reader.decodeRange(
                0,
                Math.min(FILTERED_CHUNK_SIZE, warnErrorMap.length),
                true
            );

            expect(filteredEvents).not.toBeNull();
            expect(
                (filteredEvents as NonNullable<typeof filteredEvents>).length
            ).toBeGreaterThan(0);
        }
    });

    it("should ignore KQL filter on unstructured stream", () => {
        reader.deserializeStream();

        reader.filterLogEvents(null, "some_query");
        const kqlMap = reader.getFilteredLogEventMap();

        expect(kqlMap).toBeNull();
    });

    it("should find nearest log event by timestamp", () => {
        const numEvents = reader.deserializeStream();

        const events = reader.decodeRange(0, Math.min(DECODE_CHUNK_SIZE, numEvents), false);

        expect(events).not.toBeNull();

        const [{timestamp: targetTs}] = events as NonNullable<typeof events>;
        const nearestIdx = reader.findNearestLogEventByTimestamp(targetTs);

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
