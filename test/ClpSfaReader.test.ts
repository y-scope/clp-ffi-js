import {
    afterEach,
    beforeAll,
    describe,
    expect,
    it,
} from "vitest";

import type {FileInfoArray} from "../src/clp_ffi_js/sfa/types.js";
import {
    type ClpSfaReader,
    createModule,
    loadTestData,
    type MainModule,
} from "./utils.js";


const CLP_JSON_TEST_LOG_FILES_EXPECTED_FILE_COUNT = 9;
const CLP_JSON_TEST_LOG_FILES_EXPECTED_EVENT_COUNT = 132n;
const COCKROACHDB_EXPECTED_EVENT_COUNT = 200000n;
const NUM_DECODED_EVENTS_TO_PRINT = 3;
const POSTGRESQL_EXPECTED_EVENT_COUNT = 1000000n;

let module: MainModule;

beforeAll(async () => {
    module = await createModule();
});

describe("ClpSfaReader", () => {
    let reader: ClpSfaReader | null = null;

    afterEach(() => {
        if (null !== reader) {
            reader.delete();
            reader = null;
        }
    });

    it("should read postgresql sfa archive from buffer", async () => {
        const data = await loadTestData("postgresql.clp");
        reader = new module.ClpSfaReader(data);

        expect(reader.getEventCount()).toBe(POSTGRESQL_EXPECTED_EVENT_COUNT);
    });

    it("should read cockroachdb sfa archive from buffer", async () => {
        const data = await loadTestData("cockroachdb.clp");
        reader = new module.ClpSfaReader(data);

        expect(reader.getEventCount()).toBe(COCKROACHDB_EXPECTED_EVENT_COUNT);
    });

    it("should read clp_json_test_log_files sfa archive from buffer", async () => {
        const data = await loadTestData("clp_json_test_log_files.clp");
        reader = new module.ClpSfaReader(data);

        const fileNames = reader.getFileNames() as string[];
        expect(fileNames.length).toBe(CLP_JSON_TEST_LOG_FILES_EXPECTED_FILE_COUNT);

        const fileInfos = reader.getFileInfos() as FileInfoArray;
        expect(fileInfos.length).toBe(CLP_JSON_TEST_LOG_FILES_EXPECTED_FILE_COUNT);

        fileInfos.forEach((range: FileInfoArray[number], idx: number) => {
            console.log(
                `[clp_json_test_log_files] range[${idx}] ${range.fileName}: ` +
                `[${range.logEventIdxStart}, ${range.logEventIdxEnd}) ` +
                `count=${range.logEventCount}`
            );
        });

        expect(reader.getEventCount()).toBe(CLP_JSON_TEST_LOG_FILES_EXPECTED_EVENT_COUNT);
    });

    it("should print decoded events from clp_json_test_log_files sfa archive", async () => {
        const data = await loadTestData("clp_json_test_log_files.clp");
        reader = new module.ClpSfaReader(data);

        const decodedEvents = reader.decode() as string[];
        expect(decodedEvents.length).toBe(Number(CLP_JSON_TEST_LOG_FILES_EXPECTED_EVENT_COUNT));

        const numToPrint = Math.min(NUM_DECODED_EVENTS_TO_PRINT, decodedEvents.length);
        for (let i = 0; i < numToPrint; i += 1) {
            console.log(`[clp_json_test_log_files] decoded[${i}]: ${decodedEvents[i]}`);
        }
    });
});
