import {isClpFile} from "clp-ffi-js/sfa";
import {
    describe,
    expect,
    it,
} from "vitest";

import {loadTestData} from "./utils.js";


const SFA_FIXTURE_FILENAMES = [
    "postgresql.clp",
    "cockroachdb.clp",
    "clp_json_test_log_files.clp",
];

describe("SFA utilities", () => {
    it("should detect downloaded .clp fixtures as CLP SFA archives", async () => {
        for (const filename of SFA_FIXTURE_FILENAMES) {
            const data = await loadTestData(filename);

            expect(isClpFile(data.buffer)).toBe(true);
            expect(isClpFile(data.buffer.slice(0, 4))).toBe(true);
            expect(isClpFile(data.buffer.slice(0, 2))).toBe(false);
        }
    });
});
