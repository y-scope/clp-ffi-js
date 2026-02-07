import {playwright} from "@vitest/browser-playwright";
import {defineConfig} from "vitest/config";


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
                    testTimeout: 30_000,
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
                },
            },
        ],
    },
});
