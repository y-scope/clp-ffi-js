import {
    beforeAll,
    describe,
    expect,
    it,
} from "vitest";

import {
    createModule,
    type MainModule,
} from "./utils.js";


let module: MainModule;

beforeAll(async () => {
    module = await createModule();
});

describe("Module Constants", () => {
    it("should export IrStreamType enum with correct values", () => {
        expect(module.IrStreamType.STRUCTURED.value).toBe(0);
        expect(module.IrStreamType.UNSTRUCTURED.value).toBe(1);
    });

    it("should export MERGED_KV_PAIRS keys as strings", () => {
        expect(typeof module.MERGED_KV_PAIRS_AUTO_GENERATED_KEY).toBe("string");
        expect(typeof module.MERGED_KV_PAIRS_USER_GENERATED_KEY).toBe("string");
    });
});
