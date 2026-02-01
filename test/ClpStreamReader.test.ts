import {
    beforeAll,
    describe,
    expect,
    it,
} from "vitest";

import {
    IR_STREAM_TYPE_STRUCTURED,
    IR_STREAM_TYPE_UNSTRUCTURED,
} from "./constants.js";
import {type MainModule} from "./utils/common.js";
import {createModule} from "./utils/index.js";


let module: MainModule;

beforeAll(async () => {
    module = await createModule();
});

describe("Module Constants", () => {
    it("should export IrStreamType enum with correct values", () => {
        expect(module.IrStreamType.STRUCTURED.value).toBe(IR_STREAM_TYPE_STRUCTURED);
        expect(module.IrStreamType.UNSTRUCTURED.value).toBe(IR_STREAM_TYPE_UNSTRUCTURED);
    });

    it("should export MERGED_KV_PAIRS keys with correct values", () => {
        expect(module.MERGED_KV_PAIRS_AUTO_GENERATED_KEY).toBe("auto-generated");
        expect(module.MERGED_KV_PAIRS_USER_GENERATED_KEY).toBe("user-generated");
    });
});
