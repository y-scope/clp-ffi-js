import type {
    JsonObject,
    JsonValue,
    RawLogEvent,
} from "./types.js";
import {isJsonObject} from "./utils.js";


/**
 * Single log event from a CLP archive.
 */
class LogEvent implements RawLogEvent {
    /**
     * Global log event index.
     */
    declare readonly logEventIdx: bigint;

    /**
     * Epoch timestamp.
     */
    declare readonly timestamp: bigint;

    /**
     * Serialized message string.
     */
    declare readonly message: string;

    /**
     * @param rawEvent Raw log event interface returned by the WASM binding.
     */
    constructor (rawEvent: RawLogEvent) {
        Object.assign(this, rawEvent);
    }

    /**
     * Parses the serialized message as a JSON object.
     *
     * @return The parsed object, or null if parsing fails or produces a non-object.
     */
    getKvPairs (): Readonly<JsonObject> | null {
        try {
            const kvPairs = JSON.parse(this.message) as JsonValue;
            if (false === isJsonObject(kvPairs)) {
                return null;
            }

            return kvPairs;
        } catch {
            return null;
        }
    }
}


export {LogEvent};
