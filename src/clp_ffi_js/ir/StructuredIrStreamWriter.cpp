#include <msgpack.hpp>

#include <clp_ffi_js/ClpFfiJsException.hpp>
#include <clp_ffi_js/ir/StreamWriter.hpp>
#include <clp_ffi_js/ir/StructuredIrStreamWriter.hpp>

namespace clp_ffi_js::ir
{

StructuredIrStreamWriter::StructuredIrStreamWriter(emscripten::val stream): StreamWriter{},
                                                                            m_output_writer{stream.call<emscripten::val>("getWriter")}
{
    auto serializer_result{ClpIrSerializer::create()};
    if (serializer_result.has_error()) {
        throw ClpFfiJsException{
                clp::ErrorCode::ErrorCode_Failure,
                __FILENAME__,
                __LINE__,
                std::format(
                        "Failed to create serializer: {} {}",
                        serializer_result.error().category().name(),
                        serializer_result.error().message()
                )
        };
    }
    m_serializer = std::make_unique<ClpIrSerializer>(std::move(serializer_result.value()));
}

template<typename Buffer>
auto encode_js_object_to_msgpack(emscripten::val object, msgpack::packer<Buffer>& packer) -> bool
{
    if (object.isNull() || object.isUndefined()) {
        packer.pack_nil();
    }
    else if (object.isString())
    {
        const auto str{object.as<std::string>()};
        packer.pack(str);
    }
    else if (object.isNumber())
    {
        const auto num{object.as<std::int64_t>()};
        packer.pack(num);
    }
    else if (object.isTrue() || object.isFalse())
    {
        const auto boolean{object.as<bool>()};
        packer.pack(boolean);
    }
    else if (object.isArray())
    {
        size_t length = object["length"].as<size_t>();
        packer.pack_array(length);

        for (size_t i = 0; i < length; ++i) {
            if (false == encode_js_object_to_msgpack(object[i], packer)) {
                return false;
            }
        }
    }
    else
    {
        // assume isObject
        emscripten::val keys = emscripten::val::global("Object").call<emscripten::val>("getOwnPropertyNames", object);
        size_t length = keys["length"].as<size_t>();
        packer.pack_map(length);

        for (size_t i = 0; i < length; ++i) {
            std::string key = keys[i].as<std::string>();
            packer.pack(key);

            if (false == encode_js_object_to_msgpack(object[key], packer)) {
                return false;
            }
        }
    }

    return true;
}

auto printBuffer(const uint8_t* buffer, size_t size) -> void {
    for (size_t i = 0; i < size; ++i) {
        std::cout
                  << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<int>(buffer[i]) << " ";
    }
    std::cout << std::dec << std::endl; // reset to decimal output
}

auto spanToUint8Array(const std::span<const int8_t> &span)-> emscripten::val  {
    const auto* data = reinterpret_cast<const uint8_t*>(span.data());
    size_t length = span.size();

    // Allocate JS Uint8Array and copy data in
    emscripten::val uint8Array = emscripten::val::global("Uint8Array").new_(length);
    emscripten::val memoryView = emscripten::val::module_property("HEAPU8")
        .call<emscripten::val>("subarray",
            reinterpret_cast<uintptr_t>(data),
            reinterpret_cast<uintptr_t>(data) + length
        );

    uint8Array.call<void>("set", memoryView);
    return uint8Array;
}

auto StructuredIrStreamWriter::write(emscripten::val chunk)-> void
{
    if (chunk.isNull()) {
        throw ClpFfiJsException{
                clp::ErrorCode::ErrorCode_Failure,
                __FILENAME__,
                __LINE__,
                "Chunk is null."
        };
    }

    msgpack::sbuffer buffer;
    msgpack::packer packer(buffer);

    auto pack_result{encode_js_object_to_msgpack(chunk, packer)};
    std::cout<<pack_result<<std::endl;

    auto unpacked {
        msgpack::unpack(
                buffer.data(),
                buffer.size()
        )
    };

    auto unpacked_map {unpacked.get().via.map};
    auto serializer_result{
        // FIXME
        m_serializer->serialize_msgpack_map(unpacked_map, unpacked_map)
    };

    std::cout<<"serializer_result="<<serializer_result<<std::endl;
    m_output_writer.call<emscripten::val>("write",
        spanToUint8Array(m_serializer->get_ir_buf_view())
    );
    m_serializer->clear_ir_buf();
}

auto StructuredIrStreamWriter::flush() -> void
{

}

auto StructuredIrStreamWriter::close() -> void
{
}

auto StructuredIrStreamWriter::write_ir_buf_to_output_stream()-> bool
{
    return false;
}
} // clp_ffi_js
