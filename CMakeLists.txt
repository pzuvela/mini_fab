cmake_minimum_required(VERSION 3.20)
set(CMAKE_POLICY_VERSION_MINIMUM 3.20)

project(MiniFab LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(mini_fab_tracking STATIC
	wafer/wafer_manager.cpp
)

add_library(mini_fab_equipment STATIC
	equipment/robot/sim_robot.cpp
	"equipment/process_module/sim_process_module.cpp"
)

add_library(mini_fab_engine STATIC
	"engine/process_engine.cpp"
)

foreach(target IN ITEMS
	mini_fab_tracking
	mini_fab_equipment
	mini_fab_engine
)

	target_include_directories(${target} PUBLIC
    		${CMAKE_CURRENT_SOURCE_DIR}
	)

endforeach()

add_library(mini_fab_recipes STATIC
   	persistence/sqlite_recipe_repository.cpp
	recipes/recipe_manager.cpp
)

target_include_directories(mini_fab_recipes PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(mini_fab_event_types STATIC
	equipment/event_type.cpp
)

target_include_directories(mini_fab_event_types PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)


add_library(mini_fab_events STATIC
	persistence/sqlite_event_repository.cpp
)

target_include_directories(mini_fab_events PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(mini_fab_events PUBLIC
	mini_fab_event_types
	SQLiteCpp
)

add_library(mini_fab_controller STATIC
    controller/mini_fab_controller.cpp
)

target_include_directories(mini_fab_controller PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(mini_fab_controller PUBLIC
	mini_fab_engine
	mini_fab_tracking
	mini_fab_equipment
	mini_fab_recipes
	mini_fab_events
)

add_executable(mini_fab_demo
    "app/mini_fab_demo.cpp"
)

target_link_libraries(mini_fab_demo PRIVATE
    mini_fab_engine
    mini_fab_tracking
    mini_fab_equipment
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

FetchContent_Declare(
    SQLiteCpp
    GIT_REPOSITORY https://github.com/SRombauts/SQLiteCpp.git
    GIT_TAG 3.3.3
)

set(SQLITECPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SQLITECPP_RUN_CPPLINT OFF CACHE BOOL "" FORCE)
set(SQLITECPP_RUN_CPPCHECK OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(SQLiteCpp)

target_link_libraries(mini_fab_recipes PUBLIC
    SQLiteCpp
)

# Tests
add_subdirectory(tests)

add_library(mini_fab_serialization STATIC
    web/serialization/equipment_snapshot_json.cpp
    web/serialization/equipment_events_json.cpp
    web/serialization/json_utils.cpp
    web/serialization/recipe_json.cpp
)

target_link_libraries(mini_fab_serialization PUBLIC
	mini_fab_event_types
)

target_include_directories(mini_fab_serialization PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

	FetchContent_Declare(
    		httplib
    		URL https://github.com/yhirose/cpp-httplib/archive/refs/tags/v0.58.0.zip
    		DOWNLOAD_EXTRACT_TIMESTAMP TRUE
	)

	FetchContent_MakeAvailable(httplib)


add_library(mini_fab_web_parsing STATIC
    web/parsing/recipe_parser.cpp
)

target_include_directories(mini_fab_web_parsing PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(mini_fab_web_parsing PUBLIC
    httplib::httplib
)

option(MINIFAB_BUILD_WEB "Build MiniFab web server" OFF)

if(MINIFAB_BUILD_WEB)

	find_package(Threads REQUIRED)

	add_executable(mini_fab_web
		app/mini_fab_web.cpp
		web/routes/equipment_routes.cpp
		web/routes/recipe_routes.cpp
	)

	target_link_libraries(mini_fab_web PRIVATE
	    mini_fab_engine
	    mini_fab_tracking
	    mini_fab_equipment
	    mini_fab_controller
	    mini_fab_serialization
	    mini_fab_web_parsing
	    httplib::httplib
	    Threads::Threads
	)

	add_custom_target(mini_fab_web_assets ALL
	    COMMAND ${CMAKE_COMMAND} -E copy_directory
        	"${CMAKE_CURRENT_SOURCE_DIR}/web/public"
        	"$<TARGET_FILE_DIR:mini_fab_web>/public"
    		COMMENT "Copying MiniFab web assets"
    		VERBATIM
	)

	add_dependencies(mini_fab_web_assets mini_fab_web)

	target_compile_definitions(mini_fab_web PRIVATE
    		MINIFAB_PUBLIC_DIR="$<TARGET_FILE_DIR:mini_fab_web>/public"
	)
endif()
