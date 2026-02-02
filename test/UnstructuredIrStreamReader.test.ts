import {
    afterEach,
    beforeAll,
    beforeEach,
    describe,
    expect,
    it,
} from "vitest";

import {describeCommonTests} from "./commonStreamReaderTests.js";
import {
    FILTERED_CHUNK_SIZE,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    NUM_EVENTS_UNSTRUCTURED_YARN,
    NUM_EVENTS_UNSTRUCTURED_YARN_INFO,
    NUM_EVENTS_UNSTRUCTURED_YARN_WARN_ERROR,
} from "./constants.js";
import {
    assertNonNull,
    type ClpStreamReader,
    createModule,
    createReader,
    loadTestData,
    type MainModule,
} from "./utils.js";


let module: MainModule;
let data: Uint8Array;

beforeAll(async () => {
    module = await createModule();
    data = await loadTestData("unstructured-yarn.clp.zst");
});

describe("Unstructured IR Stream: unstructured-yarn.clp.zst", () => {
    let reader: ClpStreamReader;

    beforeEach(() => {
        reader = createReader(module, data);
    });

    afterEach(() => {
        reader.delete();
    });

    describeCommonTests({
        expectedNumEvents: NUM_EVENTS_UNSTRUCTURED_YARN,
        expectedStreamType: "UNSTRUCTURED",
        getModule: () => module,
        getReader: () => reader,
    });

    it("should filter log events by log level", () => {
        reader.deserializeStream();

        reader.filterLogEvents([LOG_LEVEL_INFO]);
        const infoMap = reader.getFilteredLogEventMap();

        assertNonNull(infoMap);
        expect(infoMap.length).toBe(NUM_EVENTS_UNSTRUCTURED_YARN_INFO);
    });

    it("should decode events with useFilter=true", () => {
        reader.deserializeStream();

        reader.filterLogEvents([
            LOG_LEVEL_WARN,
            LOG_LEVEL_ERROR,
        ]);
        const warnErrorMap = reader.getFilteredLogEventMap();

        assertNonNull(warnErrorMap);
        expect(warnErrorMap.length).toBe(NUM_EVENTS_UNSTRUCTURED_YARN_WARN_ERROR);

        const filteredEvents = reader.decodeRange(
            0,
            Math.min(FILTERED_CHUNK_SIZE, warnErrorMap.length),
            true
        );

        assertNonNull(filteredEvents);
        expect(filteredEvents.length).toBe(FILTERED_CHUNK_SIZE);
    });

    it("should ignore KQL filter on unstructured stream", () => {
        reader.deserializeStream();

        reader.filterLogEvents(null, "some_query");
        const kqlMap = reader.getFilteredLogEventMap();

        expect(kqlMap).toBeNull();
    });
});
