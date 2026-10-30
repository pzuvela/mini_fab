cmake_minimum_required(VERSION 3.20)

project(MiniFab LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(mini_fab_core
    "equipment/robot/sim_robot.cpp"
    "wafer/wafer_manager.cpp"
    "engine/process_engine.cpp"
    "equipment/process_module/sim_process_module.cpp"
)

target_include_directories(mini_fab_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(mini_fab_demo
    "app/mini_fab_demo.cpp"
)

target_link_libraries(mini_fab_demo PRIVATE
    mini_fab_core
)

include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

enable_testing()

# Tests
add_subdirectory(tests)
