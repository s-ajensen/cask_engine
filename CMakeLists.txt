cmake_minimum_required(VERSION 3.14)
project(cask_engine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.4.0
)
FetchContent_MakeAvailable(Catch2)

add_library(cask_engine
    src/cask/engine/engine.cpp
    src/cask/world/abi.cpp
)
target_include_directories(cask_engine PUBLIC include src)
set_target_properties(cask_engine PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_executable(cask_engine_tests
    spec/engine/engine_spec.cpp
    spec/world/world_spec.cpp
    spec/world/abi_spec.cpp
    spec/abi/plugin_spec.cpp
    spec/abi/manifest_spec.cpp
    spec/deps/graph_spec.cpp
    spec/plugin/loader_spec.cpp
    spec/plugin/registry_spec.cpp
)
target_link_libraries(cask_engine_tests PRIVATE cask_engine Catch2::Catch2WithMain)
target_compile_definitions(cask_engine_tests PRIVATE 
    TEST_PLUGIN_DIR="${CMAKE_BINARY_DIR}"
)

include(CTest)
include(Catch)
catch_discover_tests(cask_engine_tests)

add_executable(cask src/main.cpp)
target_link_libraries(cask PRIVATE cask_engine)

add_library(minimal_plugin SHARED spec/plugin/fixtures/timing_plugin.cpp)
target_include_directories(minimal_plugin PRIVATE include src)
target_link_libraries(minimal_plugin PRIVATE cask_engine)
set_target_properties(minimal_plugin PROPERTIES PREFIX "")

find_package(OpenGL REQUIRED)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.8
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

add_library(glad STATIC plugins/cubes/glad/src/glad.c)
target_include_directories(glad PUBLIC plugins/cubes/glad/include)
set_target_properties(glad PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(cubes_plugin SHARED plugins/cubes/cubes_plugin.cpp)
target_include_directories(cubes_plugin PRIVATE include src plugins/cubes/glad/include)
target_link_libraries(cubes_plugin PRIVATE cask_engine glfw glad OpenGL::GL)
set_target_properties(cubes_plugin PROPERTIES PREFIX "")
