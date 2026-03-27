import {
    afterEach,
    describe,
    expect,
    it,
} from "vitest";

import {
    ClpArchiveReader,
    type FieldValue,
} from "clp-ffi-js/sfa";
import {
    loadTestData,
} from "./utils.js";


const CLP_JSON_TEST_LOG_FILES_EXPECTED_FILE_COUNT = 9;
const CLP_JSON_TEST_LOG_FILES_EXPECTED_EVENT_COUNT = 132n;
const COCKROACHDB_EXPECTED_EVENT_COUNT = 200000n;
const POSTGRESQL_EXPECTED_EVENT_COUNT = 1000000n;

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
        reader = ClpArchiveReader.create(data);

        expect(reader.getEventCount()).toBe(POSTGRESQL_EXPECTED_EVENT_COUNT);
    });

    it("should read cockroachdb sfa archive from buffer", async () => {
        const data = await loadTestData("cockroachdb.clp");
        reader = ClpArchiveReader.create(data);

        const data_wo_ts = await loadTestData("cockroachdb_wo_ts.clp");
        readerWoTs = ClpArchiveReader.create(data_wo_ts);

        expect(reader.getEventCount()).toBe(COCKROACHDB_EXPECTED_EVENT_COUNT);
        expect(readerWoTs.getEventCount()).toBe(COCKROACHDB_EXPECTED_EVENT_COUNT);
    });

    it("should read clp_json_test_log_files sfa archive from buffer", async () => {
        const data = await loadTestData("clp_json_test_log_files.clp");
        reader = await ClpArchiveReader.create(data);

        const fileNames = reader.getFileNames();
        expect(fileNames.length).toBe(CLP_JSON_TEST_LOG_FILES_EXPECTED_FILE_COUNT);

        const fileInfos = reader.getFileInfos();
        expect(fileInfos.length).toBe(CLP_JSON_TEST_LOG_FILES_EXPECTED_FILE_COUNT);

        expect(reader.getEventCount()).toBe(CLP_JSON_TEST_LOG_FILES_EXPECTED_EVENT_COUNT);
    });

    it("should throw when calling getEventCount after close", async () => {
        const data = await loadTestData("postgresql.clp");
        const closedReader = ClpArchiveReader.create(data);
        closedReader.close();

        expect(() => closedReader.getEventCount()).toThrow();
    });

    it("should not throw when calling close multiple times", async () => {
        const data = await loadTestData("postgresql.clp");
        const closedReader = ClpArchiveReader.create(data);
        closedReader.close();

        expect(() => {
            closedReader.close();
        }).not.toThrow();
    });
});
