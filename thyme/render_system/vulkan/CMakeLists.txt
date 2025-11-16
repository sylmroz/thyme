SET(MODULE_FILES
        $<$<CONFIG:Debug>:${CMAKE_CURRENT_SOURCE_DIR}/vulkan_debug.cppm>
        vulkan_backend.cppm
        gui.cppm
        vulkan_buffer.cppm
        vulkan_command_buffers.cppm
        vulkan_device.cppm
        vulkan_framework.cppm
        vulkan_graphic_context.cppm
        vulkan_graphic_pipeline.cppm
        vulkan_model.cppm
        vulkan_renderer.cppm
        vulkan_shader.cppm
        vulkan_swapchain.cppm
        vulkan_texture.cppm
        vulkan_uniform_buffer_object.cppm
        vulkan_utils.cppm
)

set(SRC_FILES
        vulkan_buffer.cpp
        vulkan_command_buffers.cpp
        vulkan_device.cpp
        vulkan_framework.cpp
        vulkan_graphic_pipeline.cpp
        vulkan_model.cpp
        vulkan_renderer.cpp
        vulkan_swapchain.cpp
        vulkan_texture.cpp
)

target_sources(${PROJECT_NAME}
        PUBLIC FILE_SET thyme_render_target_vulkan TYPE CXX_MODULES FILES ${MODULE_FILES}
        PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${Vulkan_INCLUDE_DIR} FILES ${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm
        PRIVATE ${SRC_FILES}
)
