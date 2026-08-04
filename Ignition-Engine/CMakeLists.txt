# Ignition CMakeLists

project(Ignition)

find_package(Vulkan REQUIRED)

file(GLOB_RECURSE IGNITION_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.inl"
)

file(GLOB_RECURSE IGNITION_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Include/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Include/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Include/*.inl"
)

add_library(Ignition SHARED ${IGNITION_SOURCES} ${IGNITION_HEADERS})

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${IGNITION_SOURCES} ${IGNITION_HEADERS})

target_compile_features(Ignition PUBLIC cxx_std_23)

set_target_properties(Ignition PROPERTIES
    CXX_EXTENSIONS OFF
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

target_include_directories(Ignition
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Include"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source"
)

target_compile_definitions(Ignition PRIVATE IGNITION_BUILD_DLL)

target_link_libraries(Ignition PRIVATE
    SDL3::SDL3-static
    Vulkan::Vulkan
PUBLIC
    spdlog::spdlog
)