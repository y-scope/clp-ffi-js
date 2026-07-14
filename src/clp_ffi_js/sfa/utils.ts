import type {
    JsonObject,
    JsonValue,
} from "./types.js";


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
const isClpJsonSingleFileArchive = (input: ArrayBuffer | ArrayBufferView): boolean => {
    const bytes = input instanceof ArrayBuffer ?
        new Uint8Array(input) :
        new Uint8Array(input.buffer, input.byteOffset, input.byteLength);

    if (bytes.byteLength < CLP_SFA_MAGIC_BYTES.length) {
        return false;
    }

    return CLP_SFA_MAGIC_BYTES.every((value, index) => bytes[index] === value);
};

/**
 * Determines whether the given value is a `JsonObject` and applies a TypeScript narrowing
 * conversion if so.
 *
 * @param value
 * @return A TypeScript type predicate indicating whether `value` is a `JsonObject`.
 */
const isJsonObject = (value: JsonValue): value is JsonObject => {
    return "object" === typeof value && null !== value && false === Array.isArray(value);
};

export {
    CLP_SFA_MAGIC_BYTES,
    isClpJsonSingleFileArchive,
    isJsonObject,
};
