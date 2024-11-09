FetchContent_Declare(subprocess URL https://github.com/benman64/subprocess/archive/1b35a4.zip)
FetchContent_MakeAvailable(subprocess)
target_include_directories(main PUBLIC ${subprocess_SOURCE_DIR}/src/cpp)
target_link_libraries(main PRIVATE subprocess)
