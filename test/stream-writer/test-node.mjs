import NodeModuleInit from "../clp-ffi-js/ClpFfiJs-node.js"
import { createWriteStream } from "node:fs"
import { pack } from "msgpackr"

const createTestWritableStream = (queueingStrategy) =>
    new WritableStream({
            _fsStream: null,
            start: function () {
                this._fsStream = createWriteStream('../assets/test.clp.zst')
            },
            write: function (chunk, controller) {
                if (controller.signal.aborted) {
                    console.log('[node] Stream aborted')
                    return
                }
                this._fsStream.write(chunk)
            },
            close: function () {
                this._fsStream.close()
                console.log('[node] Stream closed')
            },
            abort: (err) => {
                console.error('[node] Stream aborted:', err)
            }
        },
        queueingStrategy
    );

const main = async () => {
    const nodeModule = await NodeModuleInit();

    const queueingStrategy = new ByteLengthQueuingStrategy({highWaterMark: 10000});
    const queueingStrategyWithLowHighWaterMark = new ByteLengthQueuingStrategy({highWaterMark: 6290});
    // TODO: add NaN and Finite cases.
    const obj = {
        "int": 1,
        "float": 0.5,
        "boolean": true,
        "null": null,
        "string": "foo bar",
        "array": [
            "foo",
            "bar"
        ],
        "object": {
            "foo": 1,
            "baz": 0.5
        }
    };

    // console.time("node-compress");
    // const nodeStream = createTestWritableStream(queueingStrategy);
    // const nodeStreamWriter = new nodeModule.StreamWriter(nodeStream, {
    //     compressionLevel: 3
    // });
    // for (let i = 0; i < 1000000; i++) {
    //     nodeStreamWriter.write(pack(obj));
    // }
    // nodeStreamWriter.close();
    //
    // console.timeEnd("node-compress");

    console.time("node-compress-with-low-high-water-mark");
    const nodeStreamWithLowHighWaterMark = createTestWritableStream(queueingStrategyWithLowHighWaterMark);
    const nodeStreamWriterWithLowHighWaterMark = new nodeModule.StreamWriter(nodeStreamWithLowHighWaterMark, {
        compressionLevel: 3
    });
    for (let i = 0; i < 1000000; i++) {
        nodeStreamWriterWithLowHighWaterMark.write(pack(obj));
    }
    nodeStreamWriterWithLowHighWaterMark.close();

    console.timeEnd("node-compress-with-low-high-water-mark");

    // console.log(await streamWriter.desiredSize)
}

main().then(r => console.log("Test finished"))
