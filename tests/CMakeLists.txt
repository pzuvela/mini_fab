add_executable(mini_fab_tests
    process_module_tests.cpp
    robot_tests.cpp
    sim_robot_tests.cpp
    process_engine_tests.cpp
    wafer_manager_tests.cpp
    mini_fab_controller_tests.cpp
    equipment_snapshot_json_tests.cpp

    mocks/mock_robot.cpp
    mocks/blocking_mock_robot.cpp
    mocks/mock_wafer_manager.cpp
    mocks/mock_process_module.cpp
    mocks/blocking_mock_process_module.cpp
    mocks/two_stage_blocking_mock_robot.cpp
)

target_link_libraries(mini_fab_tests PRIVATE
	mini_fab_engine
	mini_fab_tracking
	mini_fab_equipment
	mini_fab_controller
	mini_fab_serialization
	GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(mini_fab_tests)
