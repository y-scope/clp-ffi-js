import {defineConfig} from "vitest/config";
import {playwright} from "@vitest/browser-playwright";


export default defineConfig({
    test: {
        globalSetup: "test/globalSetup.ts",
        projects: [
            {
                test: {
                    name: "node",
                    include: ["test/**/*.test.ts"],
                    environment: "node",
                    testTimeout: 30_000,
                },
            },
            {
                test: {
                    name: "browser",
                    include: ["test/**/*.test.ts"],
                    browser: {
                        enabled: true,
                        provider: playwright(),
                        instances: [
                            {browser: "chromium"},
                            {browser: "firefox"},
                            {browser: "webkit"},
                        ],
                        headless: true,
                    },
                    testTimeout: 30_000,
                },
            },
        ],
    },
});
