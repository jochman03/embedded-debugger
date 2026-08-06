cmake_minimum_required(VERSION 3.20)

project(embedded_debugger_linux LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
    "${CMAKE_BINARY_DIR}/bin"
)

option(BUILD_GUI "Build debugger GUI" ON)
option(BUILD_CLI "Build debugger CLI" ON)

add_subdirectory(core)

if(BUILD_GUI)
    add_subdirectory(gui)
endif()

if(BUILD_CLI)
    add_subdirectory(cli)
endif()
