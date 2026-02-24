import {constants} from "node:fs";
import {
    access,
    mkdir,
    writeFile,
} from "node:fs/promises";
import {
    TEST_DATA_DIR_URL,
    TEST_DATA_URLS,
} from "./constants.js";
import {fetchFile} from "./utils.js";

/**
 * Downloads all test data files to `TEST_DATA_DIR_URL` if they don't already exist on disk.
 *
 * This setup runs once in node environment before all test projects runs (node or browser), so
 * browser tests can fetch from local filesystem paths instead of S3.
 */
const setup = async () => {
    await mkdir(TEST_DATA_DIR_URL, {recursive: true});

    const downloads = Object.entries(TEST_DATA_URLS).map(async ([filename, url]) => {
        const dstUrl = new URL(filename, TEST_DATA_DIR_URL);
        try {
            await access(dstUrl, constants.R_OK);
            return;
        } catch {
            // File does not exist yet, continue to download.
        }

        console.log(`Downloading ${filename} from ${url}`);
        const data = await fetchFile(url);
        await writeFile(dstUrl, data);
    });

    await Promise.all(downloads);
};


export default setup;
