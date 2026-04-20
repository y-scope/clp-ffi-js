/**
 * Source file metadata from the archive's range index.
 */
interface FileInfo {
    fileName: string;
    logEventIdxStart: bigint;
    logEventIdxEnd: bigint;
    logEventCount: bigint;
}

type FieldValue =
    number |
    string |
    boolean |
    null |
    {[key: string]: FieldValue} |
    FieldValue[];

export type {
    FieldValue,
    FileInfo,
};
