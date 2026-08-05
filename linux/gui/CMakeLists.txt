cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(serial_monitor LANGUAGES CXX)

find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)

include(FetchContent)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG docking
)

FetchContent_Declare(
    implot
    GIT_REPOSITORY https://github.com/epezent/implot.git
    GIT_TAG master
)

FetchContent_MakeAvailable(imgui implot)

set(IMGUI_DIR ${imgui_SOURCE_DIR})
set(IMPLOT_DIR ${implot_SOURCE_DIR})

add_executable(serial_monitor
    src/main.cpp
    src/application.cpp
    src/serial_scan.cpp
    src/serial_port.cpp
    src/log_entry.cpp
    src/plot_manager.cpp

    src/terminal.cpp

    src/protocol_parser.cpp
    src/protocol_command.cpp
    src/protocol.cpp
    src/recording.cpp

    src/session.cpp
    src/session_manager.cpp


    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp

    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
    ${IMPLOT_DIR}/implot.cpp
    ${IMPLOT_DIR}/implot_items.cpp
)

target_include_directories(serial_monitor PRIVATE
    ${CMAKE_SOURCE_DIR}/inc
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${IMPLOT_DIR}
)

target_link_libraries(serial_monitor PRIVATE glfw OpenGL::GL)

target_compile_options(serial_monitor PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)
