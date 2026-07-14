/**
 * Source file metadata from the archive's range index.
 */
interface FileInfo {
    fileName: string;
    logEventIdxStart: bigint;
    logEventIdxEnd: bigint;
    logEventCount: bigint;
}

/**
 * Type for values in a JSON object/array.
 * Reference: https://www.json.org/json-en.html
 */
type JsonValue = null |
    string |
    number |
    boolean |
    {
        [key: string]: JsonValue;
    } |
    Array<JsonValue>;

/**
 * JSON object type.
 * Reference: https://www.json.org/json-en.html
 */
type JsonObject = {
    [key: string]: JsonValue;
};

export type {
    FileInfo,
    JsonObject,
    JsonValue,
};
