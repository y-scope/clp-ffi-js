#ifndef CLP_FFI_JS_IR_STREAMREADERDATACONTEXT_HPP
#define CLP_FFI_JS_IR_STREAMREADERDATACONTEXT_HPP

#include <memory>
#include <utility>
#include <ystdlib/containers/Array.hpp>

#include <clp/ReaderInterface.hpp>

namespace clp_ffi_js::ir {
/**
 * The data context for a `StreamReader`. It encapsulates a chain of the following resources:
 * An IR deserializer class that reads from a `clp::ReaderInterface`, which in turn reads from a
 * `clp::Array`.
 * @tparam Deserializer Type of deserializer.
 */
template <typename Deserializer>
class StreamReaderDataContext {
public:
    // Constructors
    StreamReaderDataContext(
            ystdlib::containers::Array<char>&& data_buffer,
            std::unique_ptr<clp::ReaderInterface>&& reader,
            Deserializer deserializer
    )
            : m_data_buffer{std::move(data_buffer)},
              m_reader{std::move(reader)},
              m_deserializer{std::move(deserializer)} {}

    // Disable copy constructor and assignment operator
    StreamReaderDataContext(StreamReaderDataContext const&) = delete;
    auto operator=(StreamReaderDataContext const&) -> StreamReaderDataContext& = delete;

    // Default move constructor and assignment operator
    StreamReaderDataContext(StreamReaderDataContext&&) = default;
    auto operator=(StreamReaderDataContext&&) -> StreamReaderDataContext& = default;

    // Destructor
    ~StreamReaderDataContext() = default;

    // Methods
    [[nodiscard]] auto get_deserializer() -> Deserializer& { return m_deserializer; }

    [[nodiscard]] auto get_reader() -> clp::ReaderInterface& { return *m_reader; }

private:
    ystdlib::containers::Array<char> m_data_buffer;
    std::unique_ptr<clp::ReaderInterface> m_reader;
    Deserializer m_deserializer;
};
}  // namespace clp_ffi_js::ir

#endif  // CLP_FFI_JS_IR_STREAMREADERDATACONTEXT_HPP
