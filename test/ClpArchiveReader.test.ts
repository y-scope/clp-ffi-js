import {ClpArchiveReader} from "clp-ffi-js/sfa";
import {
    afterEach,
    describe,
    expect,
    it,
} from "vitest";

import {loadTestData} from "./utils.js";


const COCKROACHDB_EXPECTED_EVENT_COUNT = 200000n;
const POSTGRESQL_EXPECTED_EVENT_COUNT = 1000000n;

describe("ClpArchiveReader", () => {
    let reader: ClpArchiveReader | null = null;

    afterEach(() => {
        if (null !== reader) {
            reader.close();
            reader = null;
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

        expect(reader.getEventCount()).toBe(COCKROACHDB_EXPECTED_EVENT_COUNT);
    });
    it("should throw when calling getEventCount after close", async () => {
        const data = await loadTestData("postgresql.clp");
        const closedReader = ClpArchiveReader.create(data);
        closedReader.close();

        expect(() => closedReader.getEventCount()).toThrow();
    });

    it("should not throw when calling close multiple times", async () => {
        const data = await loadTestData("postgresql.clp");
        reader = ClpArchiveReader.create(data);
        reader.close();

        expect(() => reader.close()).not.toThrow();
        reader = null;
    });
});
