find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)

include(FetchContent)

set(FETCHCONTENT_QUIET OFF)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG docking
    GIT_PROGRESS TRUE
)

FetchContent_Declare(
    implot
    GIT_REPOSITORY https://github.com/epezent/implot.git
    GIT_TAG master
    GIT_PROGRESS TRUE
)

FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.12.0
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(json imgui implot)

set(IMGUI_DIR ${imgui_SOURCE_DIR})
set(IMPLOT_DIR ${implot_SOURCE_DIR})

add_executable(debugger_gui
    src/main.cpp
    src/application.cpp
    src/plot.cpp
    src/plot_manager.cpp
    src/terminal.cpp
    src/recording.cpp
    src/session.cpp
    src/session_manager.cpp
    src/profile.cpp
    src/profile_manager.cpp
    src/log_entry.cpp


    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp

    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp

    ${IMPLOT_DIR}/implot.cpp
    ${IMPLOT_DIR}/implot_items.cpp
)

target_include_directories(debugger_gui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${IMPLOT_DIR}
)

target_link_libraries(debugger_gui PRIVATE
    debugger_core
    glfw
    OpenGL::GL
    nlohmann_json::nlohmann_json
)

target_compile_options(debugger_gui PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)
