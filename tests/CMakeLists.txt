add_executable(melodee_tests core_tests.cpp)
target_link_libraries(melodee_tests PRIVATE melodee_core)

if(MSVC)
    target_compile_options(melodee_tests PRIVATE /utf-8)
endif()

add_test(NAME melodee_core_tests COMMAND melodee_tests)
