set(MODULE_FILES
        glfw_context.cppm
        glfw_window.cppm
)

SET(SOURCE_FILES
)

target_sources(${PROJECT_NAME}
        PUBLIC FILE_SET platform TYPE CXX_MODULES FILES ${MODULE_FILES}
        PRIVATE ${SOURCE_FILES}
)
