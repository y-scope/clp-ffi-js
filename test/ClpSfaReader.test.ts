import {
    afterEach,
    beforeAll,
    describe,
    expect,
    it,
} from "vitest";

import {
    type ClpSfaReader,
    createModule,
    loadTestData,
    type MainModule,
} from "./utils.js";


const COCKROACH_EXPECTED_EVENT_COUNT = 200000n;
const CLP_JSON_TEST_LOG_FILES_EXPECTED_EVENT_COUNT = 132n;
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
        reader = module.ClpSfaReader.create(data);

        expect(reader.getFileNames()).toEqual(["postgresql.log"]);
        expect(reader.getEventCount()).toBe(POSTGRESQL_EXPECTED_EVENT_COUNT);
    });

    it("should read cockroach sfa archive from buffer", async () => {
        const data = await loadTestData("cockroach.clp");
        reader = module.ClpSfaReader.create(data);

        expect(reader.getFileNames()).toEqual(["cockroach.log"]);
        expect(reader.getEventCount()).toBe(COCKROACH_EXPECTED_EVENT_COUNT);
    });

    it("should read clp_json_test_log_files sfa archive from buffer", async () => {
        const data = await loadTestData("clp_json_test_log_files.clp");
        reader = module.ClpSfaReader.create(data);

        expect(reader.getFileNames().length).toBe(9);
        expect(reader.getEventCount()).toBe(CLP_JSON_TEST_LOG_FILES_EXPECTED_EVENT_COUNT);
    });
});
