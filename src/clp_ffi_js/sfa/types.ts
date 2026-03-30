/**
 * Source file metadata from the archive's range index.
 */
interface FileInfo {
    fileName: string;
    logEventIdxStart: bigint;
    logEventIdxEnd: bigint;
    logEventCount: bigint;
}

type FileInfoArray = FileInfo[];

export type {
    FileInfo,
    FileInfoArray,
};
