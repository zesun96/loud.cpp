cmake_minimum_required(VERSION 3.10)

project(main)

# Export compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Enable FetchContent
include(FetchContent)

# Prepare executable
add_executable(main src/main.cpp)
set_target_properties(main PROPERTIES OUTPUT_NAME "loud")
set_target_properties(main PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Define output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Add JSON lib
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_MakeAvailable(json)
target_link_libraries(main PRIVATE nlohmann_json::nlohmann_json)

# Add whisper lib
FetchContent_Declare(whisper URL https://github.com/ggerganov/whisper.cpp/archive/refs/tags/v1.7.1.tar.gz)
FetchContent_MakeAvailable(whisper)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
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

target_include_directories(main PUBLIC ${whisper_SOURCE_DIR}/include)
target_include_directories(main PUBLIC ${whisper_SOURCE_DIR}/ggml/include)

# Link whisper
if(APPLE)
    target_link_libraries(main PRIVATE
        whisper
        ggml
        "-framework CoreFoundation"
        "-framework Metal"
        "-framework CoreML"
    )
else()
    target_link_libraries(main PRIVATE
        whisper
        ggml
    )
endif()

# Add sherpa-onnx
if(WIN32)
    set(SHERPA_URL https://github.com/k2-fsa/sherpa-onnx/releases/download/v1.10.30/sherpa-onnx-v1.10.30-win-x64-shared.tar.bz2)
elseif(UNIX AND NOT APPLE)
    set(SHERPA_URL https://github.com/k2-fsa/sherpa-onnx/releases/download/v1.10.30/sherpa-onnx-v1.10.30-linux-x64-shared.tar.bz2)
elseif(APPLE)
    set(SHERPA_URL https://github.com/k2-fsa/sherpa-onnx/releases/download/v1.10.30/sherpa-onnx-v1.10.30-osx-universal2-shared.tar.bz2)
endif()
FetchContent_Declare(
    sherpa
    URL ${SHERPA_URL}
)
FetchContent_MakeAvailable(sherpa)
target_include_directories(main PUBLIC ${sherpa_SOURCE_DIR}/include)

# Link sherpa
target_link_directories(main PRIVATE ${sherpa_SOURCE_DIR}/lib)
target_link_libraries(main PRIVATE
    sherpa-onnx-c-api
    cargs
    $<$<NOT:$<PLATFORM_ID:Windows>>:onnxruntime>
)

# Copy sherpa lib/ and bin/
add_custom_command(TARGET main POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${sherpa_SOURCE_DIR}/lib $<TARGET_FILE_DIR:main>
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${sherpa_SOURCE_DIR}/bin $<TARGET_FILE_DIR:main>
)
