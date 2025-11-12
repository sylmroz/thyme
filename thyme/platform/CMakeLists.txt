set(MODULE_FILES
        window.cppm
        window_event_handler.cppm
        window_settings.cppm
        imgui_context.cppm
)

set(SRC_FILES
        window_settings.cpp
)

target_sources(${PROJECT_NAME}
        PUBLIC FILE_SET thyme_platform TYPE CXX_MODULES FILES ${MODULE_FILES}
        PRIVATE ${SRC_FILES}
)

add_subdirectory(glfw)
