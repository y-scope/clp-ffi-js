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


const COCKROACHDB_EXPECTED_EVENT_COUNT = 200000n;
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
});
