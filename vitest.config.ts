import {defineConfig} from "vitest/config";


export default defineConfig({
    test: {
        testTimeout: 30_000,
        projects: [
            "vitest.node.config.ts",
            "vitest.browser.config.ts",
        ],
    },
});
