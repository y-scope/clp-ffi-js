import ModuleInit from "./cmake-build-debug/ClpFfiJs-node.js"
import {createWriteStream} from "node:fs"

const main = async () => {
    const module = await ModuleInit()

    console.time("compress")
    const queueingStrategy = new ByteLengthQueuingStrategy({highWaterMark: 1024000000});
    const stream = new WritableStream({
            _fsStream: null,
            start: function () {
                this._fsStream = createWriteStream('test.clp.zst')
            },
            write: function (chunk, controller) {
                if (controller.signal.aborted) {
                    console.log('Stream aborted')
                    return
                }
                this._fsStream.write(chunk)
            },
            close: function () {
                this._fsStream.close()
                console.log('Stream closed')
            },
            abort: (err) => {
                console.error('Stream aborted:', err)
            }
        },
        queueingStrategy
    )
    const streamWriter = new module.StreamWriter(stream)

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
    }
    for (let i = 0; i < 1000000; i++) {
        streamWriter.write(obj)
    }
    streamWriter.close()

    console.timeEnd('compress')
    // console.log(await streamWriter.desiredSize)
}

main()