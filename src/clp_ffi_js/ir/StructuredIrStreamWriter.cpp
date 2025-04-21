#include <msgpack.hpp>
#include <streaming_compression/zstd/Compressor.hpp>

#include <spdlog/spdlog.h>

#include <clp_ffi_js/ClpFfiJsException.hpp>
#include <clp_ffi_js/ir/StreamWriter.hpp>
#include <clp_ffi_js/ir/StructuredIrStreamWriter.hpp>

namespace clp_ffi_js::ir {
namespace {
constexpr std::string_view cWriterOptionsCompressionLevel{"compressionLevel"};

class WebStreamWriter final: public clp::WriterInterface {
public:
    // Delete default constructor to disable direct instantiation.
    WebStreamWriter() = delete;

    explicit WebStreamWriter(emscripten::val stream)
            : WriterInterface{},
              m_writer{stream.call<emscripten::val>("getWriter")} {}

    // Delete copy & move constructors and assignment operators
    WebStreamWriter(WebStreamWriter const&) = delete;
    WebStreamWriter(WebStreamWriter&&) = delete;
    auto operator=(WebStreamWriter const&) -> WebStreamWriter& = delete;
    auto operator=(WebStreamWriter&&) -> WebStreamWriter& = delete;

    // Destructor
    ~WebStreamWriter() override = default;

    void write(char const* data, size_t data_length) override {
        auto const uint8Array{emscripten::val::global("Uint8Array").new_(data_length)};
        emscripten::val memoryView{emscripten::typed_memory_view(data_length, data)};

        uint8Array.call<void>("set", memoryView);
        m_writer.call<void>("write", uint8Array);
    }

    void flush() override { return; }

    clp::ErrorCode try_seek_from_begin(size_t pos) override { return clp::ErrorCode_Unsupported; }

    clp::ErrorCode try_seek_from_current(off_t offset) override {
        return clp::ErrorCode_Unsupported;
    }

    clp::ErrorCode try_get_pos(size_t& pos) const override { return clp::ErrorCode_Unsupported; }

private:
    emscripten::val m_writer;
};
}  // namespace

StructuredIrStreamWriter::StructuredIrStreamWriter(
        emscripten::val const& stream,
        WriterOptions const& writer_options
)
        : StreamWriter{},
          m_output_writer{std::make_unique<WebStreamWriter>(stream)} {
    int compression_level{clp::streaming_compression::zstd::cDefaultCompressionLevel};
    if (writer_options.hasOwnProperty(cWriterOptionsCompressionLevel.data())) {
        compression_level = writer_options[cWriterOptionsCompressionLevel.data()].as<int>();
    }

    m_msgpack_buf.reserve(cDefaultMsgpackBufferSizeLimit);

    m_writer = std::make_unique<clp::streaming_compression::zstd::Compressor>();
    m_writer->open(
            *m_output_writer,
            compression_level
    );

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

auto StructuredIrStreamWriter::write(emscripten::val chunk) -> void {
    emscripten::val packed_user_gen_handle = emscripten::val::global("msgpackr").call<emscripten::val>("pack", chunk);

    size_t const packed_user_gen_handle_length = packed_user_gen_handle["length"].as<int>();
    m_msgpack_buf.resize(packed_user_gen_handle_length);
    const emscripten::val memoryView{
            emscripten::typed_memory_view(packed_user_gen_handle_length, m_msgpack_buf.data())
    };
    memoryView.call<void>("set", packed_user_gen_handle);

    auto const unpacked_user_gen_handle{msgpack::unpack(
            reinterpret_cast<char const*>(m_msgpack_buf.data()),
            m_msgpack_buf.size()
    )};
    auto const unpacked_user_gen_map{unpacked_user_gen_handle.get().via.map};
    m_msgpack_buf.clear();

    // FIXME: this should come from the arg 'chunk' as well
    msgpack::object_map auto_gen_map{0, nullptr};

    auto const serializer_result{
            m_serializer->serialize_msgpack_map(auto_gen_map, unpacked_user_gen_map)
    };
    if (false == serializer_result) {
        throw ClpFfiJsException{
                clp::ErrorCode::ErrorCode_Failure,
                __FILENAME__,
                __LINE__,
                std::format("Failed to serialize msgpack map")
        };
    }

    if (cDefaultIrBufferSizeLimit < get_ir_buf_size()) {
        write_ir_buf_to_output_stream();
    }
}

auto StructuredIrStreamWriter::flush() -> void {
    write_ir_buf_to_output_stream();
    m_writer->flush();
}

auto StructuredIrStreamWriter::close() -> void {
    write_ir_buf_to_output_stream();
    m_writer->close();

    // FIXME: handle any read on this after close()
    m_serializer.reset(nullptr);
}

auto StructuredIrStreamWriter::write_ir_buf_to_output_stream() const -> void {
    auto const ir_buf_view{m_serializer->get_ir_buf_view()};
    m_writer->write(reinterpret_cast<char const*>(ir_buf_view.data()), ir_buf_view.size());
    m_serializer->clear_ir_buf();
}
}  // namespace clp_ffi_js::ir
