import type {
    JsonObject,
    JsonValue,
} from "./types.js";
import {isJsonObject} from "./utils.js";


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
     * @return The key-value pairs, or null if the message is not a JSON object.
     */
    getKvPairs (): Readonly<JsonObject> | null {
        try {
            const kvPairs = JSON.parse(this.message) as JsonValue;
            if (false === isJsonObject(kvPairs)) {
                console.warn(
                    `Log event message is not a JSON object. Log event index: ${this.logEventIdx}.`
                );

                return null;
            }

            return kvPairs;
        } catch (error) {
            console.warn(
                `Failed to parse log event message as JSON. Log event index: ${this.logEventIdx}.`,
                error
            );

            return null;
        }
    }
}


export {LogEvent};
