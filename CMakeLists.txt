cmake_minimum_required(VERSION 3.10)
project(loud)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Used by clangd intellisense
# set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


# Define output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Create our executable
add_executable(loud src/main.cpp)


# Force static libraries globally
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)

# Fetch whisper.cpp
include(FetchContent)
set(WHISPER_URL https://github.com/ggerganov/whisper.cpp/archive/refs/tags/v1.7.1.zip)
FetchContent_Declare(
    whisper
    URL ${WHISPER_URL}
)

# Configure whisper.cpp options before making it available
set(WHISPER_CCACHE OFF CACHE BOOL "")
set(GGML_CCACHE OFF CACHE BOOL "")
set(WHISPER_BUILD_TESTS OFF CACHE BOOL "")
set(WHISPER_BUILD_EXAMPLES OFF CACHE BOOL "")
set(WHISPER_BUILD_SERVER OFF CACHE BOOL "")

if(APPLE)
    set(WHISPER_COREML ON CACHE BOOL "")
    set(WHISPER_COREML_ALLOW_FALLBACK ON CACHE BOOL "")
    set(GGML_METAL ON CACHE BOOL "")
endif()

# Now make whisper available with our options
FetchContent_MakeAvailable(whisper)

# Fetch sherpa-onnx# Determine Sherpa-ONNX URL based on OS
if(WIN32)
    set(SHERPA_URL https://github.com/k2-fsa/sherpa-onnx/releases/download/v1.10.30/sherpa-onnx-v1.10.30-win-x64-shared.tar.bz2)
elseif(UNIX AND NOT APPLE)
    set(SHERPA_URL https://github.com/k2-fsa/sherpa-onnx/releases/download/v1.10.30/sherpa-onnx-v1.10.30-linux-x64-shared.tar.bz2)
elseif(APPLE)
    set(SHERPA_URL https://github.com/k2-fsa/sherpa-onnx/releases/download/v1.10.30/sherpa-onnx-v1.10.30-osx-universal2-shared.tar.bz2)
endif()

# Fetch sherpa-onnx
FetchContent_Declare(
    sherpa
    URL ${SHERPA_URL}
)

# Fetch sherpa-onnx
FetchContent_Declare(
    sherpa
    URL ${SHERPA_URL}
)

# Make sherpa available
FetchContent_MakeAvailable(sherpa)

# Add sherpa-onnx include and link directories
target_include_directories(loud PRIVATE ${sherpa_SOURCE_DIR}/include/sherpa-onnx/c-api)
target_include_directories(loud PRIVATE ${sherpa_SOURCE_DIR}/include)
target_link_directories(loud PRIVATE ${sherpa_SOURCE_DIR}/lib)

target_link_libraries(loud PRIVATE
    sherpa-onnx-c-api
    cargs
    $<$<NOT:$<PLATFORM_ID:Windows>>:onnxruntime>
)


add_custom_command(TARGET loud POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${sherpa_SOURCE_DIR}/bin $<TARGET_FILE_DIR:loud>
)

# Copy sherpa shared libs to bin folder
add_custom_command(TARGET loud POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${sherpa_SOURCE_DIR}/lib $<TARGET_FILE_DIR:loud>
)


# Include directories
target_include_directories(loud PRIVATE
    ${whisper_SOURCE_DIR}
    ${whisper_SOURCE_DIR}/include
    ${whisper_SOURCE_DIR}/ggml/include
)

# Link libraries
if(APPLE)
    target_link_libraries(loud PRIVATE
        whisper
        ggml
        "-framework CoreFoundation"
        "-framework Metal"
        "-framework CoreML"
    )
else()
    target_link_libraries(loud PRIVATE
        whisper
        ggml
    )
endif()