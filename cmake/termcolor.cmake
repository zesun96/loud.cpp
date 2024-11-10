FetchContent_Declare(termcolor URL https://github.com/ikalnytskyi/termcolor/archive/89f200.zip)
FetchContent_MakeAvailable(termcolor)
target_include_directories(main PUBLIC ${termcolor_SOURCE_DIR}/include)
target_link_libraries(main PRIVATE termcolor)


