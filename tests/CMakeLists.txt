include(GoogleTest)

# mainApp_test.cpp is preserved for the expanded layout implementation, 
# most of which I believe we changed to be contained directly in UI.
# It requires keyboardLayout.hpp, which is not present in this checkout,
# so it is not part of the runnable suite.
add_executable(unit_tests
    pianoKeyboard_test.cpp
    chordLogic_test.cpp
    pianoLayout_test.cpp
    ui_test.cpp
    uiComponent_test.cpp
    chordProgression_test.cpp
)

target_link_libraries(unit_tests PRIVATE
    GTest::gtest_main
    app_core
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
