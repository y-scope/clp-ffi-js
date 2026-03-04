import {
    afterEach,
    beforeAll,
    describe,
    expect,
    it,
} from "vitest";

import {
    createModule,
    loadTestData,
    type MainModule,
} from "./utils.js";


let module: MainModule;

beforeAll(async () => {
    module = await createModule();
});

describe("ClpSfaReader", () => {
    // eslint-disable-next-line @typescript-eslint/no-explicit-any
    let reader: any = null;

    afterEach(() => {
        if (null !== reader) {
            reader.delete();
            reader = null;
        }
    });

    it("should read sfa archive from buffer", async () => {
        const data = await loadTestData("../../sfa.clp");
        reader = module.ClpSfaReader.create(data, "SFA");

        expect(reader.getArchiveId()).toBe("SFA");
        expect(reader.getEventCount()).toBe(1000000);
    });
});
