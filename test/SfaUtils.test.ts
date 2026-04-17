import {
    CLP_SFA_MAGIC_BYTES,
    isClpFile,
} from "clp-ffi-js/sfa";
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
            for (let len = 0; CLP_SFA_MAGIC_BYTES.length >= len; len += 1) {
                expect(isClpFile(data.slice(0, len).buffer)).toBe(
                    len === CLP_SFA_MAGIC_BYTES.length
                );
            }
        }
    });
});
