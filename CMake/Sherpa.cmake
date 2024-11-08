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