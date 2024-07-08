cmake_minimum_required(VERSION 3.22)

project(ClpIrV1Decoder)

# Download and extract Boost
set(BOOST_VERSION "1.85.0")
string(REPLACE "." "_" BOOST_ARCHIVE_VERSION_PART ${BOOST_VERSION})

include(ExternalProject)
ExternalProject_Add(
        Boost
        URL "https://archives.boost.io/release/${BOOST_VERSION}/source/boost_${BOOST_ARCHIVE_VERSION_PART}.tar.gz"
        PREFIX "${CMAKE_BINARY_DIR}/boost"
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        LOG_DOWNLOAD ON
)
ExternalProject_Get_Property(Boost SOURCE_DIR)
set(BOOST_EXTRACT_DIR "${SOURCE_DIR}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++23")
set(CMAKE_CXX_STANDARD 23)

set(CMAKE_EXECUTABLE_SUFFIX ".js")

add_executable(
        ClpIrV1Decoder

        src/ClpIrV1Decoder.cpp

        src/submodules/clp/components/core/src/clp/ffi/ir_stream/decoding_methods.cpp
        src/submodules/clp/components/core/src/clp/ir/LogEventDeserializer.cpp
        src/submodules/clp/components/core/src/clp/ReadOnlyMemoryMappedFile.cpp
        src/submodules/clp/components/core/src/clp/ReaderInterface.cpp
        src/submodules/clp/components/core/src/clp/streaming_compression/zstd/Decompressor.cpp
        src/submodules/clp/components/core/src/clp/TimestampPattern.cpp

        src/submodules/fmt/src/format.cc

        src/submodules/zstd/lib/common/entropy_common.c
        src/submodules/zstd/lib/common/error_private.c
        src/submodules/zstd/lib/common/fse_decompress.c
        src/submodules/zstd/lib/common/zstd_trace.c
        src/submodules/zstd/lib/common/xxhash.c
        src/submodules/zstd/lib/common/zstd_common.c
        src/submodules/zstd/lib/decompress/huf_decompress.c
        src/submodules/zstd/lib/decompress/zstd_ddict.c
        src/submodules/zstd/lib/decompress/zstd_decompress_block.c
        src/submodules/zstd/lib/decompress/zstd_decompress.c
)

set_target_properties(ClpIrV1Decoder PROPERTIES COMPILE_FLAGS "-O3")
set_target_properties(ClpIrV1Decoder PROPERTIES LINK_FLAGS    "-s EXPORTED_FUNCTIONS=['_malloc','_free'] -s MODULARIZE=1 -s EXPORT_ES6=1 -s ENVIRONMENT=worker -lembind -sALLOW_MEMORY_GROWTH=1 -sWASM_BIGINT -flto -O3 --closure 1")

target_compile_definitions(ClpIrV1Decoder PUBLIC SPDLOG_FMT_EXTERNAL=1)

target_include_directories(
        ClpIrV1Decoder
        PRIVATE
        ${BOOST_EXTRACT_DIR}
        src/
        src/submodules/clp/components/core/src
        src/submodules/clp/components/core/src/clp
        src/submodules/clp/components/core/submodules
        src/submodules/fmt/include
        src/submodules/spdlog/include
        src/submodules/zstd/lib
)
