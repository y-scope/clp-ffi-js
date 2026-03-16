/**
 * Source file metadata from the archive's range index.
 */
interface FileInfo {
    fileName: string;
    logEventIdxStart: number;
    logEventIdxEnd: number;
    logEventCount: number;
}

type FileInfoArray = FileInfo[];

export type {
    FileInfo,
    FileInfoArray,
};
