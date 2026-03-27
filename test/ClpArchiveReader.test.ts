import {ClpArchiveReader} from "clp-ffi-js/sfa";
import {
    afterEach,
    describe,
    expect,
    it,
} from "vitest";

import {loadTestData} from "./utils.js";


const CLP_JSON_TEST_LOG_FILES_EXPECTED_FILE_COUNT = 9;
const CLP_JSON_TEST_LOG_FILES_EXPECTED_EVENT_COUNT = 132n;
const COCKROACHDB_EXPECTED_EVENT_COUNT = 200000n;
const POSTGRESQL_EXPECTED_EVENT_COUNT = 1000000n;

describe("ClpArchiveReader", () => {
    let reader: ClpArchiveReader | null = null;
    let readerWoTs: ClpArchiveReader | null = null;

    const createReaderFromArchive = async (archiveFilename: string): Promise<ClpArchiveReader> => {
        const data = await loadTestData(archiveFilename);
        return ClpArchiveReader.create(data);
    };

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
        reader = await createReaderFromArchive("postgresql.clp");

        expect(reader.getEventCount()).toBe(POSTGRESQL_EXPECTED_EVENT_COUNT);
    });

    it("should read cockroachdb sfa archive from buffer", async () => {
        reader = await createReaderFromArchive("cockroachdb.clp");
        readerWoTs = await createReaderFromArchive("cockroachdb_wo_ts.clp");

        expect(reader.getEventCount()).toBe(COCKROACHDB_EXPECTED_EVENT_COUNT);
        expect(readerWoTs.getEventCount()).toBe(COCKROACHDB_EXPECTED_EVENT_COUNT);
    });

    it("should read clp_json_test_log_files sfa archive from buffer", async () => {
        reader = await createReaderFromArchive("clp_json_test_log_files.clp");

        const fileNames = reader.getFileNames();
        expect(fileNames.length).toBe(CLP_JSON_TEST_LOG_FILES_EXPECTED_FILE_COUNT);

        const fileInfos = reader.getFileInfos();
        expect(fileInfos.length).toBe(CLP_JSON_TEST_LOG_FILES_EXPECTED_FILE_COUNT);

        expect(reader.getEventCount()).toBe(CLP_JSON_TEST_LOG_FILES_EXPECTED_EVENT_COUNT);
    });

    it("should throw when calling getEventCount after close", async () => {
        const closedReader = await createReaderFromArchive("postgresql.clp");
        closedReader.close();

        expect(() => closedReader.getEventCount()).toThrow();
    });

    it("should not throw when calling close multiple times", async () => {
        const closedReader = await createReaderFromArchive("postgresql.clp");
        closedReader.close();

        expect(() => {
            closedReader.close();
        }).not.toThrow();
    });
});
