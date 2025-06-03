import initModule from '../clp-ffi-js/ClpFfiJs-worker.js'
import { pack } from 'https://cdn.jsdelivr.net/npm/msgpackr@1.10.1/+esm'

function createTestWritableStream(queueingStrategy) {
    const chunks = []

    return new WritableStream({
        start() {
            console.log('[worker] Stream started')
        },
        write(chunk) {
            chunks.push(chunk)
        },
        close() {
            const blob = new Blob(chunks, { type: 'application/octet-stream' })
            console.log(`[worker] Stream closed. Final size: ${blob.size} bytes`)

            // Send back to main thread for optional download
            postMessage({ type: 'done', blob })
        },
        abort(err) {
            console.error('[worker] Stream aborted:', err)
            postMessage({ type: 'error', message: err.message })
        }
    }, queueingStrategy)
}

const runTest = async () => {
    const module = await initModule()

    const queueingStrategy = new ByteLengthQueuingStrategy({ highWaterMark: 1024000000 })
    const stream = createTestWritableStream(queueingStrategy)

    const writer = new module.StreamWriter(stream, { compressionLevel: 3 })

    const obj = {
        int: 1,
        float: 0.5,
        boolean: true,
        null: null,
        string: 'foo bar',
        array: ['foo', 'bar'],
        object: { foo: 1, baz: 0.5 }
    }

    for (let i = 0; i < 100_000; i++) {
        writer.write(pack(obj))
    }

    writer.close()
}

runTest().catch(err => {
    postMessage({ type: 'error', message: err.message })
    console.error('[worker] Error running test:', err)
})
