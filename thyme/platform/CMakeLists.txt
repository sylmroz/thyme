target_sources(${PROJECT_NAME} PUBLIC FILE_SET platform TYPE CXX_MODULES FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/platform_context.cppm
        ${CMAKE_CURRENT_SOURCE_DIR}/window.cppm
        imgui_context.cppm
)

add_subdirectory(glfw)
