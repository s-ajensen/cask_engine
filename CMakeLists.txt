cmake_minimum_required(VERSION 3.14)
project(cask_redux)

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

add_library(cask_redux
    src/engine.cpp
)
target_include_directories(cask_redux PUBLIC src)

add_executable(cask_redux_tests
    spec/engine_spec.cpp
)
target_link_libraries(cask_redux_tests PRIVATE cask_redux Catch2::Catch2WithMain)

include(CTest)
include(Catch)
catch_discover_tests(cask_redux_tests)
