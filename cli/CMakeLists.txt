add_executable(melodee main.cpp)
target_link_libraries(melodee PRIVATE melodee_core)

if(MSVC)
    target_compile_options(melodee PRIVATE /W4 /utf-8)
else()
    target_compile_options(melodee PRIVATE -Wall -Wextra)
endif()
