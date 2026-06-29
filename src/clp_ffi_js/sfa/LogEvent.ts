import type {FieldValue} from "./types.js";


/**
 * Single log event from a CLP archive.
 */
class LogEvent {
    readonly logEventIdx: bigint;

    readonly timestamp: bigint;

    readonly message: string;

    /**
     * @param logEventIdx Global log event index.
     * @param timestamp Epoch timestamp.
     * @param message Serialized message string.
     */
    constructor (logEventIdx: bigint, timestamp: bigint, message: string) {
        this.logEventIdx = logEventIdx;
        this.timestamp = timestamp;
        this.message = message;
    }

    /**
     * Returns the key-value pairs of this log event by parsing the message as JSON.
     *
     * @return The key-value pairs, or null if the message is not valid JSON.
     */
    getKvPairs (): Readonly<{[key: string]: FieldValue}> | null {
        try {
            return JSON.parse(this.message) as {[key: string]: FieldValue};
        } catch {
            return null;
        }
    }
}

export {LogEvent};
