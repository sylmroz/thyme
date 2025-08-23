target_sources(${PROJECT_NAME} PUBLIC FILE_SET platform TYPE CXX_MODULES FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/glfw_context.cppm
        ${CMAKE_CURRENT_SOURCE_DIR}/glfw_window.cppm
)
