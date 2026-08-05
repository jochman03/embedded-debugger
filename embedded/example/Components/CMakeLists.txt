add_library(project_components INTERFACE)

add_subdirectory(
    "${CMAKE_CURRENT_LIST_DIR}/../../debugger"
    "${CMAKE_CURRENT_BINARY_DIR}/debugger"
)

file(GLOB COMPONENT_DIRECTORIES
    CONFIGURE_DEPENDS
    LIST_DIRECTORIES true
    "${CMAKE_CURRENT_SOURCE_DIR}/*"
)

list(SORT COMPONENT_DIRECTORIES)

foreach(COMPONENT_DIRECTORY IN LISTS COMPONENT_DIRECTORIES)
    if(IS_DIRECTORY "${COMPONENT_DIRECTORY}"
       AND EXISTS "${COMPONENT_DIRECTORY}/CMakeLists.txt")
        add_subdirectory("${COMPONENT_DIRECTORY}")
    endif()
endforeach()
