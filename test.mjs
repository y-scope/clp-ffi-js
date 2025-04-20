import ModuleInit from "./cmake-build-debug/ClpFfiJs-node.js"
import {createWriteStream} from "node:fs"
import { compress } from '@mongodb-js/zstd';


const main = async () => {
    const module = await ModuleInit()

    const stream = new WritableStream({
        start: async function () {
            this._fsStream = createWriteStream('test.clp.zst')
        },
        write: async function (chunk) {
            const compressed = await compress(chunk)
            this._fsStream.write(compressed)
        },
        close: () => {
            console.log('Stream closed')
        },
        abort: (err) => {
            console.error('Stream aborted:', err)
        }
    })

    const streamWriter = new module.StreamWriter(stream)

    await streamWriter.write({
        message: 'hello',
        ts: 123,
        data: [1, 2, 3],
        isObject: true,
        isArray: false,
        undefined: undefined,
        null: null
    })

    // console.log(await streamWriter.desiredSize)
}

main()