set(MODULE_FILES
        platform_context.cppm
        window.cppm
        imgui_context.cppm
)

set(SRC_FILES

)

target_sources(${PROJECT_NAME}
        PUBLIC FILE_SET platform TYPE CXX_MODULES FILES ${MODULE_FILES}
        PRIVATE ${SRC_FILES}
)

add_subdirectory(glfw)
