cmake_minimum_required(VERSION 3.28)

target_sources(${PROJECT_NAME} PUBLIC FILE_SET thyme_render_target_vulkan TYPE CXX_MODULES FILES
        $<$<CONFIG:Debug>:${CMAKE_CURRENT_SOURCE_DIR}/vulkan_debug.cppm>
        ${CMAKE_CURRENT_SOURCE_DIR}/vulkan_device.cppm
        ${CMAKE_CURRENT_SOURCE_DIR}/vulkan_framework.cppm
)
