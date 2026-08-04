# Sandbox CMakeLists

project(Sandbox)

file(GLOB_RECURSE SANDBOX_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.inl"
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

target_link_libraries(Sandbox PRIVATE Ignition)

set_target_properties(Sandbox PROPERTIES
    CXX_EXTENSIONS OFF
    BUILD_RPATH "$ORIGIN"
    INSTALL_RPATH "$ORIGIN"
    VS_DEBUGGER_WORKING_DIRECTORY "${BASE_OUTPUT_DIRECTORY}"
)