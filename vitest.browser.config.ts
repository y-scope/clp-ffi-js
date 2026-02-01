import {
    dirname,
    resolve,
} from "node:path";
import {fileURLToPath} from "node:url";

import {playwright} from "@vitest/browser-playwright";
import {defineConfig} from "vitest/config";


const projectDir = dirname(fileURLToPath(import.meta.url));
const browserUtilsPath = resolve(projectDir, "test/utils/browser.ts");


export default defineConfig({
    plugins: [
        {
            enforce: "pre" as const,
            name: "redirect-utils-to-browser",

            /**
             * Redirects imports of utils/index to browser utils.
             *
             * @param source The import source path.
             * @return The browser utils path if the source is a utils index import, null otherwise.
             */
            resolveId: (source: string) => {
                if ("./utils/index.js" === source || source.endsWith("/utils/index.js")) {
                    return browserUtilsPath;
                }

                return null;
            },
        },
    ],
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
    },
});
