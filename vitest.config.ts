import {defineConfig} from "vitest/config";
import {playwright} from "@vitest/browser-playwright";


export default defineConfig({
    test: {
        globalSetup: "tests/globalSetup.ts",
        projects: [
            {
                test: {
                    name: "node",
                    include: ["tests/**/*.test.ts"],
                    environment: "node",
                    testTimeout: 30_000,
                },
            },
            {
                test: {
                    name: "browser",
                    include: ["tests/**/*.test.ts"],
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
