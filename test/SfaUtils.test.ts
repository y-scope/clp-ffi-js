import {
    CLP_SFA_MAGIC_BYTES,
    isClpJsonSingleFileArchive,
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

            expect(isClpJsonSingleFileArchive(data)).toBe(true);

            // Prefixes shorter than the full magic header must be rejected.
            for (let len = 0; CLP_SFA_MAGIC_BYTES.length >= len; len += 1) {
                expect(isClpJsonSingleFileArchive(data.slice(0, len))).toBe(
                    len === CLP_SFA_MAGIC_BYTES.length
                );
            }
        }
    });

    it("should reject buffers that don't start with the CLP SFA magic bytes", () => {
        expect(isClpJsonSingleFileArchive(new ArrayBuffer(0))).toBe(false);

        // eslint-disable-next-line @stylistic/array-element-newline, no-magic-numbers
        const nonClp = new Uint8Array([0x00, 0x01, 0x02, 0x03, 0x04, 0x05]);

        expect(isClpJsonSingleFileArchive(nonClp)).toBe(false);
        expect(isClpJsonSingleFileArchive(nonClp.buffer)).toBe(false);
    });

    it("should detect magic bytes at the start of a sliced view with a non-zero offset", () => {
        const magicHeaderCustomOffset = 123;
        const prefixedBuffer = new Uint8Array(magicHeaderCustomOffset + CLP_SFA_MAGIC_BYTES.length);

        prefixedBuffer.set(CLP_SFA_MAGIC_BYTES, magicHeaderCustomOffset);

        const slicedView = prefixedBuffer.subarray(magicHeaderCustomOffset);

        expect(isClpJsonSingleFileArchive(slicedView)).toBe(true);
        expect(isClpJsonSingleFileArchive(slicedView.buffer)).toBe(false);
    });
});
