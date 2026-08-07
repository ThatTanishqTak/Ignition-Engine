# Ignition CMakeLists

project(Ignition)

find_package(Vulkan REQUIRED)

file(GLOB_RECURSE IGNITION_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp"
)

file(GLOB_RECURSE IGNITION_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Include/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Include/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Include/*.inl"
)

add_library(Ignition SHARED ${IGNITION_SOURCES} ${IGNITION_HEADERS})

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${IGNITION_SOURCES} ${IGNITION_HEADERS})

target_compile_features(Ignition PUBLIC cxx_std_23)

if(MSVC)
    target_compile_options(Ignition PRIVATE /W4 /permissive- /Zc:__cplusplus /MP)
    target_compile_options(Ignition PRIVATE /w14251)
else()
    target_compile_options(Ignition PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(Ignition PROPERTIES
    CXX_EXTENSIONS OFF
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

target_include_directories(Ignition
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Include"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source"
)

target_compile_definitions(Ignition PRIVATE 
    IGNITION_BUILD_DLL
    VMA_STATIC_VULKAN_FUNCTIONS=1
    VMA_DYNAMIC_VULKAN_FUNCTIONS=0
    VMA_VULKAN_VERSION=1004000
)

target_link_libraries(Ignition PRIVATE
    SDL3::SDL3-static
    Vulkan::Vulkan
    GPUOpen::VulkanMemoryAllocator
    spdlog::spdlog
)