# Editor CMakeLists

project(Editor)

file(GLOB_RECURSE EDITOR_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp"
)

file(GLOB_RECURSE EDITOR_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Include/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Include/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Include/*.inl"
)

add_executable(Editor ${EDITOR_SOURCES} ${EDITOR_HEADERS})

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${EDITOR_SOURCES} ${EDITOR_HEADERS})

target_include_directories(Editor PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Include"
)

if(MSVC)
    target_compile_options(Editor PRIVATE /W4 /permissive- /Zc:__cplusplus /MP)
else()
    target_compile_options(Editor PRIVATE -Wall -Wextra -Wpedantic)
endif()

# The editor is privileged: it links ImGui/ImGuizmo directly instead of going through Ignition::UI
target_link_libraries(Editor PRIVATE Ignition imguizmo)

add_dependencies(Editor IgnitionShaders)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Assets")
    add_custom_command(TARGET Editor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/Assets"
            "${BASE_OUTPUT_DIRECTORY}/Assets"
        COMMENT "Copying Editor assets"
        VERBATIM
    )
endif()

set_target_properties(Editor PROPERTIES
    CXX_EXTENSIONS OFF
    BUILD_RPATH "$ORIGIN"
    INSTALL_RPATH "$ORIGIN"
    VS_DEBUGGER_WORKING_DIRECTORY "${BASE_OUTPUT_DIRECTORY}"
)