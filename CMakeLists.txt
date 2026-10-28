  add_library(mini_fab_core
    sim_robot.cpp
    mock_robot.cpp
    wafer_manager.cpp
    process_engine.cpp
    sim_process_module.cpp
    mock_process_module.cpp
)

target_include_directories(mini_fab_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(mini_fab_demo
    mini_fab_demo.cpp
)

target_link_libraries(mini_fab_demo PRIVATE
    mini_fab_core
)

include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

enable_testing()

# Tests
add_subdirectory(tests)