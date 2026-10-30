add_executable(mini_fab_tests
    process_module_tests.cpp
    robot_tests.cpp
    process_engine_tests.cpp
    wafer_manager_tests.cpp

    mocks/mock_robot.cpp
    mocks/mock_wafer_manager.cpp
    mocks/mock_process_module.cpp
    mocks/blocking_mock_process_module.cpp
)

target_link_libraries(mini_fab_tests PRIVATE
    mini_fab_core
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(mini_fab_tests)
