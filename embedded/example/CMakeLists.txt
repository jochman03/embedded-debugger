cmake_minimum_required(VERSION 3.22)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

get_filename_component(
    FIRMWARE_NAME
    "${CMAKE_CURRENT_SOURCE_DIR}"
    NAME
)

project(${FIRMWARE_NAME} LANGUAGES C)

option(BUILD_FIRMWARE "Build STM32 firmware" OFF)
option(BUILD_TESTING "Build unit tests" OFF)

if(BUILD_TESTING)
    enable_testing()

    include(FetchContent)

    FetchContent_Declare(
        unity
        GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
        GIT_TAG v2.6.1
        GIT_SHALLOW TRUE
    )

    FetchContent_MakeAvailable(unity)
endif()

# Components
add_subdirectory(Components)

if(BUILD_FIRMWARE)
    enable_language(ASM)

    add_executable(${PROJECT_NAME})

    add_subdirectory(cmake/stm32cubemx)
    include(Config/project-config.cmake)

    file(GLOB APP_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/App/Src/*.c"
    )

    file(GLOB PLATFORM_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/Platform/Src/*.c"
    )
    
    target_sources(${PROJECT_NAME} PRIVATE
        ${APP_SOURCES}
        ${PLATFORM_SOURCES}
    )

    target_include_directories(${PROJECT_NAME} PRIVATE
        App/Inc
        Platform/Inc
    )

    target_link_libraries(${PROJECT_NAME}
        stm32cubemx
        project_components
    )
    if(OPENOCD_TARGET STREQUAL "")
        message(FATAL_ERROR
            "OPENOCD_TARGET is empty. Set it in cmake/project-config.cmake.")
    endif()

    configure_file(
        Config/openocd.cfg.in
        "${CMAKE_BINARY_DIR}/openocd.cfg"
        @ONLY
    )
endif()