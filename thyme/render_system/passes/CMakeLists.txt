SET(MODULE_FILES
        mypass.cppm
        passes.cppm
)

set(SRC_FILES
)

target_sources(${PROJECT_NAME}
        PUBLIC FILE_SET thyme_render_target_passes TYPE CXX_MODULES FILES ${MODULE_FILES}
        PRIVATE ${SRC_FILES}
)
