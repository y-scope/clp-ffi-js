/**
 * CLP JSON single-file archive (SFA) magic bytes.
 */
const CLP_SFA_MAGIC_BYTE_0 = 0xFD;
const CLP_SFA_MAGIC_BYTE_1 = 0x2F;
const CLP_SFA_MAGIC_BYTE_2 = 0xC5;
const CLP_SFA_MAGIC_BYTE_3 = 0x30;

const CLP_SFA_MAGIC_BYTES = [
    CLP_SFA_MAGIC_BYTE_0,
    CLP_SFA_MAGIC_BYTE_1,
    CLP_SFA_MAGIC_BYTE_2,
    CLP_SFA_MAGIC_BYTE_3,
] as const;

/**
 * Detects whether a buffer is a CLP JSON single-file archive (SFA).
 *
 * @param buffer Buffer containing the archive bytes.
 * @return `true` if the buffer starts with the CLP SFA magic bytes.
 */
const isClpFile = (buffer: ArrayBuffer): boolean => {
    const header = new Uint8Array(buffer.slice(0, CLP_SFA_MAGIC_BYTES.length));

    return CLP_SFA_MAGIC_BYTES.every((value, index) => header[index] === value);
};

export {
    CLP_SFA_MAGIC_BYTES,
    isClpFile,
};
