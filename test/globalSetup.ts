import fs from "node:fs";
import path from "node:path";
import {fileURLToPath} from "node:url";


const TEST_DATA_URLS: Readonly<Record<string, string>> = {
    "structured-cockroachdb.clp.zst": "https://yscope.s3.us-east-2.amazonaws.com/sample-logs/cockroachdb.clp.zst",
    "unstructured-yarn.clp.zst": "https://yscope.s3.us-east-2.amazonaws.com/sample-logs/yarn-ubuntu-resourcemanager-ip-172-31-17-135.log.1.clp.zst",
};

const TEST_DATA_DIR = path.join(
    path.dirname(fileURLToPath(import.meta.url)),
    "data"
);

/**
 * Downloads all test data files to `test/data/` if they don't already exist on disk.
 * This runs once before all test projects (node and browser), so browser tests can fetch
 * from Vite's local dev server instead of S3.
 */
const setup = async () => {
    fs.mkdirSync(TEST_DATA_DIR, {recursive: true});

    const downloads = Object.entries(TEST_DATA_URLS).map(async ([filename, url]) => {
        const filePath = path.join(TEST_DATA_DIR, filename);
        if (fs.existsSync(filePath)) {
            return;
        }

        console.log(`Downloading ${filename} from ${url}`);
        const response = await fetch(url);
        const data = new Uint8Array(await response.arrayBuffer());
        fs.writeFileSync(filePath, data);
    });

    await Promise.all(downloads);
};


export default setup;
