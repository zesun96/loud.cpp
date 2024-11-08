cmake_minimum_required(VERSION 3.10)
project(loud)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Force static libraries globally
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)

# Fetch whisper.cpp
include(FetchContent)
FetchContent_Declare(
    whisper
    GIT_REPOSITORY https://github.com/ggerganov/whisper.cpp.git
    GIT_TAG v1.7.1
)

# Configure whisper.cpp options before making it available
set(WHISPER_CCACHE OFF CACHE BOOL "")
set(GGML_CCACHE OFF CACHE BOOL "")
set(WHISPER_BUILD_TESTS OFF CACHE BOOL "")
set(WHISPER_BUILD_EXAMPLES OFF CACHE BOOL "")
set(WHISPER_BUILD_SERVER OFF CACHE BOOL "")
set(WHISPER_COREML ON CACHE BOOL "")
set(GGML_METAL ON CACHE BOOL "")

# Now make whisper available with our options
FetchContent_MakeAvailable(whisper)

# Create our executable
add_executable(loud src/main.cpp)

# Disable deprecated declarations warning
target_compile_options(loud PRIVATE -Wno-deprecated-declarations)

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