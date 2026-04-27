/**
 * Starting byte sequence that identifies a CLP JSON single-file archive (SFA).
 */
const CLP_SFA_MAGIC_BYTES = [
    // eslint-disable-next-line no-magic-numbers, @stylistic/array-element-newline
    0xFD, 0x2F, 0xC5, 0x30,
] as const;

/**
 * Detects whether a buffer is a CLP JSON single-file archive (SFA).
 *
 * @param buffer Buffer containing the archive bytes.
 * @return `true` if the buffer starts with the CLP SFA magic bytes.
 */
const isClpFile = (buffer: ArrayBuffer): boolean => {
    if (buffer.byteLength < CLP_SFA_MAGIC_BYTES.length) {
        return false;
    }
    const header = new Uint8Array(buffer, 0, CLP_SFA_MAGIC_BYTES.length);

    return CLP_SFA_MAGIC_BYTES.every((value, index) => header[index] === value);
};

export {
    CLP_SFA_MAGIC_BYTES,
    isClpFile,
};
