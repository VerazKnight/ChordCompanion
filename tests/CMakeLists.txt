include(GoogleTest)

add_executable(unit_tests
    pianoKeyboard_test.cpp
    # Add module test source files here.
)

target_link_libraries(unit_tests PRIVATE
    GTest::gtest_main
    app_core
    # Add module libraries as needed.
)

gtest_discover_tests(unit_tests
    PROPERTIES LABELS "unit"
)

add_custom_target(check
    COMMAND ${CMAKE_CTEST_COMMAND}
        --output-on-failure
        --no-tests=error
        -L unit
        -C $<CONFIG>
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS unit_tests
    USES_TERMINAL
)
