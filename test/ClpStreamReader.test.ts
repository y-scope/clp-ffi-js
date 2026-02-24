import {
    beforeAll,
    describe,
    expect,
    it,
} from "vitest";

import {
    createModule,
    createReader,
    loadTestData,
    type MainModule,
} from "./utils.js";
import {
    IR_STREAM_TYPE_STRUCTURED,
    IR_STREAM_TYPE_UNSTRUCTURED,
} from "./constants.js";


let module: MainModule;

beforeAll(async () => {
    module = await createModule();
});

describe("Module Constants", () => {
    it("should export IrStreamType enum with correct values", () => {
        expect(module.IrStreamType.STRUCTURED.value).toBe(IR_STREAM_TYPE_STRUCTURED);
        expect(module.IrStreamType.UNSTRUCTURED.value).toBe(IR_STREAM_TYPE_UNSTRUCTURED);
    });

    it("should export MERGED_KV_PAIRS keys as strings", () => {
        expect(typeof module.MERGED_KV_PAIRS_AUTO_GENERATED_KEY).toBe("string");
        expect(typeof module.MERGED_KV_PAIRS_USER_GENERATED_KEY).toBe("string");
    });
});

describe("ClpStreamReader", () => {
    it("should create a reader for a structured IR stream", async () => {
        const data = await loadTestData("structured-cockroachdb.clp.zst");
        const reader = createReader(module, data);

        expect(reader.getIrStreamType()).toBe(module.IrStreamType.STRUCTURED);
    });

    it("should create a reader for an unstructured IR stream", async () => {
        const data = await loadTestData("unstructured-yarn.clp.zst");
        const reader = createReader(module, data);

        expect(reader.getIrStreamType()).toBe(module.IrStreamType.UNSTRUCTURED);
    });
});
