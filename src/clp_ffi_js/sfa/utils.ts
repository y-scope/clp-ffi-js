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
 * @param input Buffer or view containing the archive bytes.
 * @return `true` if the input starts with the CLP SFA magic bytes.
 */
const isClpFile = (input: ArrayBuffer | ArrayBufferView): boolean => {
    const bytes = input instanceof ArrayBuffer
        ? new Uint8Array(input)
        : new Uint8Array(input.buffer, input.byteOffset, input.byteLength);

    if (bytes.byteLength < CLP_SFA_MAGIC_BYTES.length) {
        return false;
    }

    return CLP_SFA_MAGIC_BYTES.every((value, index) => bytes[index] === value);
};

export {
    CLP_SFA_MAGIC_BYTES,
    isClpFile,
};
