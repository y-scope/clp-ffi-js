/**
 * Source file metadata from the archive's range index.
 */
interface FileInfo {
    fileName: string;
    logEventIdxStart: number;
    logEventIdxEnd: number;
    logEventCount: number;
}

export type {FileInfo};
