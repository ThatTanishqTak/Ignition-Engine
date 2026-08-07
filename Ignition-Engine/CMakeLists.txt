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
PUBLIC
    GLM_FORCE_DEPTH_ZERO_TO_ONE
    GLM_FORCE_RADIANS
)

target_link_libraries(Ignition PRIVATE
    SDL3::SDL3-static
    Vulkan::Vulkan
    GPUOpen::VulkanMemoryAllocator
    spdlog::spdlog
PUBLIC
    glm::glm
)

find_program(SLANGC_EXECUTABLE
    NAMES slangc
    HINTS
        "$ENV{VULKAN_SDK}/bin"
        "$ENV{VULKAN_SDK}/Bin"
)

if(NOT SLANGC_EXECUTABLE)
    message(FATAL_ERROR "slangc not found. Install the Vulkan SDK >= 1.3.296 (bundles Slang), or set VULKAN_SDK.")
endif()

file(GLOB IGNITION_SHADER_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.slang"
)

set(IGNITION_SHADER_OUTPUT_DIRECTORY "${BASE_OUTPUT_DIRECTORY}/Shaders")

set(IGNITION_SHADER_BINARIES "")
foreach(SHADER_SOURCE ${IGNITION_SHADER_SOURCES})
    get_filename_component(SHADER_NAME ${SHADER_SOURCE} NAME_WE)
    set(SHADER_OUTPUT "${IGNITION_SHADER_OUTPUT_DIRECTORY}/${SHADER_NAME}.spv")

    add_custom_command(
        OUTPUT ${SHADER_OUTPUT}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${IGNITION_SHADER_OUTPUT_DIRECTORY}"
        COMMAND ${SLANGC_EXECUTABLE}
            ${SHADER_SOURCE}
            -target spirv
            -matrix-layout-column-major
            -fvk-use-entrypoint-name
            $<$<CONFIG:Debug>:-g>
            -o ${SHADER_OUTPUT}
        DEPENDS ${SHADER_SOURCE}
        COMMENT "Compiling shader ${SHADER_NAME}.slang -> ${SHADER_NAME}.spv"
        VERBATIM
    )

    list(APPEND IGNITION_SHADER_BINARIES ${SHADER_OUTPUT})
endforeach()

add_custom_target(IgnitionShaders
    DEPENDS ${IGNITION_SHADER_BINARIES}
    SOURCES ${IGNITION_SHADER_SOURCES}
)

source_group("Shaders" FILES ${IGNITION_SHADER_SOURCES})