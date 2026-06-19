cmake_minimum_required(VERSION 3.21)

project(cpplin
    VERSION 0.1.0
    DESCRIPTION "C++ LIN bus library — RELAY-conformant, ISO 26262 ASIL-B"
    HOMEPAGE_URL "https://github.com/SoundMatt/cpp-LIN"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Strict warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(
        -Wall -Wextra -Wpedantic
        -Wshadow -Wnon-virtual-dtor
        -Wold-style-cast -Wcast-align
        -Wunused -Woverloaded-virtual
        -Wno-unused-parameter
    )
endif()
if(MSVC)
    add_compile_options(
        /W4
        /wd4100   # unreferenced formal parameter
        /wd4996   # deprecated CRT functions
    )
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(cmake/FetchDeps.cmake)

# ── Library ───────────────────────────────────────────────────────────────────

add_library(cpplin_lib STATIC
    src/relay.cpp
    src/lin.cpp
    src/virtual/bus.cpp
    src/safety/e2e.cpp
    src/ldf/parser.cpp
    src/master/node.cpp
    src/slave/node.cpp
)

target_include_directories(cpplin_lib
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

find_package(Threads REQUIRED)
target_link_libraries(cpplin_lib PUBLIC Threads::Threads)

# ── Tests ─────────────────────────────────────────────────────────────────────

option(CPPLIN_BUILD_TESTS "Build tests" ON)
if(CPPLIN_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ── CLI binary ────────────────────────────────────────────────────────────────

option(CPPLIN_BUILD_CLI "Build RELAY convert CLI" ON)
if(CPPLIN_BUILD_CLI)
    add_subdirectory(cli)
endif()

message(STATUS "cpp-LIN v${PROJECT_VERSION} — C++${CMAKE_CXX_STANDARD} / ${CMAKE_BUILD_TYPE}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
