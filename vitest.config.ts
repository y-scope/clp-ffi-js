import {defineConfig} from "vitest/config";


export default defineConfig({
    test: {
        globalSetup: "test/globalSetup.ts",
        include: ["test/**/*.test.ts"],
        environment: "node",
        testTimeout: 30_000,
    },
});
