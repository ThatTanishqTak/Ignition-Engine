# Vendor CMakeLists

set(CMAKE_FOLDER "Vendor")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Vendor")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Vendor")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Vendor")

set(BUILD_SHARED_LIBS OFF)

# SDL3 -------
set(SDL_SHARED OFF)
set(SDL_STATIC ON)
set(SDL_STATIC_PIC ON)
set(SDL_TEST_LIBRARY OFF)
set(SDL_TESTS OFF)
set(SDL_EXAMPLES OFF)
set(SDL_INSTALL OFF)
set(SDL_DISABLE_UNINSTALL ON)

add_subdirectory(SDL SYSTEM EXCLUDE_FROM_ALL)
# --------------

# VulkanMemoryAllocator -------
add_subdirectory(VulkanMemoryAllocator SYSTEM EXCLUDE_FROM_ALL)
# --------------

# glm ---------
set(GLM_BUILD_TESTS OFF)
set(GLM_BUILD_LIBRARY OFF)
set(GLM_BUILD_INSTALL OFF)

add_subdirectory(glm SYSTEM EXCLUDE_FROM_ALL)
# --------------

# Dear ImGui (docking branch) -------
find_package(Vulkan REQUIRED)

add_library(imgui STATIC
    imgui/imgui.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/backends/imgui_impl_sdl3.cpp
    imgui/backends/imgui_impl_vulkan.cpp
)

target_include_directories(imgui SYSTEM PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends"
)

target_link_libraries(imgui PRIVATE SDL3::SDL3-static Vulkan::Vulkan)

set_target_properties(imgui PROPERTIES POSITION_INDEPENDENT_CODE ON)
# --------------

# stb (header-only) -------
add_library(stb INTERFACE)
target_include_directories(stb SYSTEM INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/stb")
# --------------

# entt (header-only) -------
add_library(entt INTERFACE)
target_include_directories(entt SYSTEM INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/entt/single_include")
# --------------

# spdlog -------
set(SPDLOG_BUILD_SHARED OFF)
set(SPDLOG_BUILD_PIC ON)
set(SPDLOG_BUILD_EXAMPLE OFF)
set(SPDLOG_BUILD_EXAMPLE_HO OFF)
set(SPDLOG_BUILD_TESTS OFF)
set(SPDLOG_BUILD_TESTS_HO OFF)
set(SPDLOG_BUILD_BENCH OFF)
set(SPDLOG_INSTALL OFF)
set(SPDLOG_USE_STD_FORMAT ON)

add_subdirectory(spdlog SYSTEM EXCLUDE_FROM_ALL)
# --------------