# Sandbox CMakeLists

project(Sandbox)

file(GLOB_RECURSE SANDBOX_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp"
)

file(GLOB_RECURSE SANDBOX_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Include/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Include/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Include/*.inl"
)

add_executable(Sandbox ${SANDBOX_SOURCES} ${SANDBOX_HEADERS})

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SANDBOX_SOURCES} ${SANDBOX_HEADERS})

target_include_directories(Sandbox PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Include"
)

if(MSVC)
    target_compile_options(Sandbox PRIVATE /W4 /permissive- /Zc:__cplusplus /MP)
else()
    target_compile_options(Sandbox PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_link_libraries(Sandbox PRIVATE Ignition imgui)

add_dependencies(Sandbox IgnitionShaders)

add_custom_command(TARGET Sandbox POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/Assets"
        "${BASE_OUTPUT_DIRECTORY}/Assets"
    COMMENT "Copying Sandbox assets"
    VERBATIM
)

set_target_properties(Sandbox PROPERTIES
    CXX_EXTENSIONS OFF
    BUILD_RPATH "$ORIGIN"
    INSTALL_RPATH "$ORIGIN"
    VS_DEBUGGER_WORKING_DIRECTORY "${BASE_OUTPUT_DIRECTORY}"
)