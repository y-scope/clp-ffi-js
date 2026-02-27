import {
    afterEach,
    beforeAll,
    describe,
    expect,
    it,
} from "vitest";

import {
    createModule,
    type MainModule,
} from "./utils.js";


type ClpSfaReaderHandle = {
    delete: () => void;
    getArchiveId: () => string;
    getEventCount: () => bigint | number;
};

let module: MainModule;

const isNodeRuntime = (): boolean => {
    // eslint-disable-next-line @typescript-eslint/no-unnecessary-condition
    return "undefined" !== typeof process && "string" === typeof process.versions?.node;
};

const getBasename = (filePath: string): string => {
    const normalized = filePath.replaceAll("\\", "/");
    const parts = normalized.split("/");

    return parts.at(-1) ?? normalized;
};

beforeAll(async () => {
    module = await createModule();
});

describe("ClpSfaReader", () => {
    let reader: ClpSfaReaderHandle | null = null;

    afterEach(() => {
        if (null !== reader) {
            reader.delete();
            reader = null;
        }
    });

    it("should export ClpSfaReader constructor", () => {
        const maybeCtor = (module as unknown as {ClpSfaReader?: unknown}).ClpSfaReader;
        expect(typeof maybeCtor).toBe("function");
    });

    it("should throw when opening a non-existent archive path", () => {
        const missingPath = "/tmp/does-not-exist.sfa";
        const ClpSfaReaderCtor = (module as unknown as {ClpSfaReader: new (p: string) => ClpSfaReaderHandle})
            .ClpSfaReader;

        expect(() => new ClpSfaReaderCtor(missingPath)).toThrow();
    });

    const sfaPath = true === isNodeRuntime() ? process.env.CLP_FFI_JS_TEST_SFA_PATH : undefined;
    it.skipIf(
        false === isNodeRuntime() || "string" !== typeof sfaPath || 0 === sfaPath.length
    )("should read archive id and event count from an SFA file when configured", () => {
        const nonNullSfaPath = sfaPath as string;

        const ClpSfaReaderCtor = (module as unknown as {ClpSfaReader: new (p: string) => ClpSfaReaderHandle})
            .ClpSfaReader;
        reader = new ClpSfaReaderCtor(nonNullSfaPath);

        const archiveId = reader.getArchiveId();
        const eventCount = reader.getEventCount();

        expect(archiveId.length).toBeGreaterThan(0);
        expect(archiveId).toContain(getBasename(nonNullSfaPath));
        expect(Number(eventCount)).toBeGreaterThanOrEqual(0);
    });
});
