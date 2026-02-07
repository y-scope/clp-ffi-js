import {
    expect,
    it,
} from "vitest";

import {
    DECODE_CHUNK_SIZE,
    FILTERED_CHUNK_SIZE,
    LOG_LEVEL_INFO,
} from "./constants.js";
import {
    assertNonNull,
    type ClpStreamReader,
    type MainModule,
} from "./utils.js";


interface CommonTestParams {
    expectedNumEvents: number;
    expectedStreamType: "STRUCTURED" | "UNSTRUCTURED";
    getModule: () => MainModule;
    getReader: () => ClpStreamReader;
}

/**
 * Registers tests for stream metadata, deserialization, and basic decoding.
 *
 * @param params Test parameters and accessors for the module and reader.
 */
const describeStreamBasics = (params: CommonTestParams) => {
    const {expectedNumEvents, expectedStreamType, getModule, getReader} = params;

    it(`should report IR stream type as ${expectedStreamType}`, () => {
        const streamType = getReader().getIrStreamType();

        expect(streamType.value).toBe(
            getModule().IrStreamType[expectedStreamType].value
        );
    });

    it("should return metadata as an object", () => {
        const metadata = getReader().getMetadata();

        expect(metadata).not.toBeNull();
        expect(typeof metadata).toBe("object");
    });

    it("should deserialize stream and return the expected event count", () => {
        const numEvents = getReader().deserializeStream();

        expect(numEvents).toBe(expectedNumEvents);
    });

    it("should match getNumEventsBuffered with deserialized count", () => {
        const reader = getReader();
        const numEvents = reader.deserializeStream();
        const numBuffered = reader.getNumEventsBuffered();

        expect(numBuffered).toBe(numEvents);
    });

    it("should decode first events with expected fields", () => {
        const reader = getReader();
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
};

/**
 * Registers tests for range decoding, filtering, and timestamp lookup.
 *
 * @param params Test parameters and accessors for the module and reader.
 */
const describeDecodeAndFilter = (params: CommonTestParams) => {
    const {getReader} = params;

    it("should decode last events", () => {
        const reader = getReader();
        const numEvents = reader.deserializeStream();

        const lastEvents = reader.decodeRange(
            Math.max(0, numEvents - FILTERED_CHUNK_SIZE),
            numEvents,
            false
        );

        assertNonNull(lastEvents);
        expect(lastEvents.length).toBe(FILTERED_CHUNK_SIZE);
    });

    it("should reset filter when passing null", () => {
        const reader = getReader();
        reader.deserializeStream();

        reader.filterLogEvents([LOG_LEVEL_INFO]);
        reader.filterLogEvents(null);
        const resetMap = reader.getFilteredLogEventMap();

        expect(resetMap).toBeNull();
    });

    it("should find nearest log event by timestamp", () => {
        const reader = getReader();
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
        const reader = getReader();
        const numEvents = reader.deserializeStream();

        const invalidRange = reader.decodeRange(
            numEvents,
            numEvents + 1,
            false
        );

        expect(invalidRange).toBeNull();
    });
};

/**
 * Registers shared `it()` tests for any IR stream reader. Must be called inside a `describe()`
 * block that sets up the reader via `beforeEach`/`afterEach`.
 *
 * @param params Test parameters and accessors for the module and reader.
 */
const describeCommonTests = (params: CommonTestParams) => {
    describeStreamBasics(params);
    describeDecodeAndFilter(params);
};


export type {CommonTestParams};
export {describeCommonTests};
