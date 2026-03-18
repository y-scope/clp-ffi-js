import {
    afterEach,
    beforeAll,
    describe,
    expect,
    it,
} from "vitest";

import {ClpArchiveReader} from "../src/clp_ffi_js/sfa/ClpArchiveReader.js";
import type {FieldValue} from "../src/clp_ffi_js/sfa/types.js";
import {
    createModule,
    loadTestData,
    type MainModule,
} from "./utils.js";


const CLP_JSON_TEST_LOG_FILES_EXPECTED_FILE_COUNT = 9;
const CLP_JSON_TEST_LOG_FILES_EXPECTED_EVENT_COUNT = 132n;
const COCKROACHDB_EXPECTED_EVENT_COUNT = 200000n;
const COCKROACHDB_TIMESTAMP_CHECK_COUNT = 1000;
const MAX_LOG_EVENT_INDEX_CHECKS = 1000;
const POSTGRESQL_EXPECTED_EVENT_COUNT = 1000000n;

let module: MainModule;

beforeAll(async () => {
    module = await createModule();
});

const assertLogEventIndices = (reader: ClpArchiveReader, expectedCount: bigint): void => {
    const decodedEvents = reader.decodeAll();
    expect(decodedEvents.length).toBe(Number(expectedCount));

    const numChecks = Math.min(decodedEvents.length, MAX_LOG_EVENT_INDEX_CHECKS);
    for (let i = 0; i < numChecks; i += 1) {
        const event = decodedEvents[i];
        expect(event).toBeDefined();
        expect(event?.logEventIdx).toBe(BigInt(i));
    }
};

const parseTimestampFieldToMs = (value: FieldValue): bigint | null => {
    if ("number" === typeof value) {
        return BigInt(Math.trunc(value));
    }
    if ("string" === typeof value) {
        if (value.includes(".")) {
            const asNumber = Number.parseFloat(value);
            if (Number.isFinite(asNumber)) {
                return BigInt(Math.trunc(asNumber * 1000));
            }
            return null;
        }
        if (/^-?\d+$/.test(value)) {
            return BigInt(value);
        }
    }
    return null;
};

describe("ClpArchiveReader", () => {
    let reader: ClpArchiveReader | null = null;
    let readerWoTs: ClpArchiveReader | null = null;

    afterEach(() => {
        if (null !== reader) {
            reader.close();
            reader = null;
        }
        if (null !== readerWoTs) {
            readerWoTs.close();
            readerWoTs = null;
        }
    });

    it("should read postgresql sfa archive from buffer", async () => {
        const data = await loadTestData("postgresql.clp");
        reader = ClpArchiveReader.create(module, data);

        expect(reader.getEventCount()).toBe(POSTGRESQL_EXPECTED_EVENT_COUNT);
        assertLogEventIndices(reader, POSTGRESQL_EXPECTED_EVENT_COUNT);
    });

    it("should read cockroachdb sfa archive from buffer", async () => {
        const data = await loadTestData("cockroachdb.clp");
        reader = ClpArchiveReader.create(module, data);

        const data_wo_ts = await loadTestData("cockroachdb_wo_ts.clp");
        readerWoTs = ClpArchiveReader.create(module, data_wo_ts);

        expect(reader.getEventCount()).toBe(COCKROACHDB_EXPECTED_EVENT_COUNT);
        expect(readerWoTs.getEventCount()).toBe(COCKROACHDB_EXPECTED_EVENT_COUNT);
        assertLogEventIndices(reader, COCKROACHDB_EXPECTED_EVENT_COUNT);
        assertLogEventIndices(readerWoTs, COCKROACHDB_EXPECTED_EVENT_COUNT);

        const decodedEvents = reader.decodeAll();
        const decodedEventsWoTs = readerWoTs.decodeAll();
        expect(decodedEvents.length).toBe(Number(COCKROACHDB_EXPECTED_EVENT_COUNT));
        expect(decodedEventsWoTs.length).toBe(decodedEvents.length);

        for (let i = 0; i < decodedEvents.length; i += 1) {
            expect(decodedEventsWoTs[i]?.message).toBe(decodedEvents[i]?.message);
        }

        for (let i = 0; i < Math.min(COCKROACHDB_TIMESTAMP_CHECK_COUNT, decodedEvents.length); i += 1) {
            const event = decodedEvents[i];
            const eventWoTs = decodedEventsWoTs[i];
            expect(event).toBeDefined();
            expect(eventWoTs).toBeDefined();
            expect(eventWoTs?.logEventIdx).toBe(event?.logEventIdx);
            expect(eventWoTs?.timestamp).toBe(0n);
            const kvPairs = event?.getKvPairs();
            expect(kvPairs).not.toBeNull();
            const timestampField = kvPairs?.["timestamp"];
            expect(timestampField).toBeDefined();
            const parsedTimestamp = parseTimestampFieldToMs(timestampField as FieldValue);
            expect(parsedTimestamp).not.toBeNull();
            expect(parsedTimestamp).toBe(event?.timestamp);
        }
    });

    it("should read clp_json_test_log_files sfa archive from buffer", async () => {
        const data = await loadTestData("clp_json_test_log_files.clp");
        reader = ClpArchiveReader.create(module, data);

        const fileNames = reader.getFileNames();
        expect(fileNames.length).toBe(CLP_JSON_TEST_LOG_FILES_EXPECTED_FILE_COUNT);

        const fileInfos = reader.getFileInfos();
        expect(fileInfos.length).toBe(CLP_JSON_TEST_LOG_FILES_EXPECTED_FILE_COUNT);

        expect(reader.getEventCount()).toBe(CLP_JSON_TEST_LOG_FILES_EXPECTED_EVENT_COUNT);
        assertLogEventIndices(reader, CLP_JSON_TEST_LOG_FILES_EXPECTED_EVENT_COUNT);
    });
});
