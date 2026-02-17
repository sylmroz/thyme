set(MODULE_FILES
        gui.cppm
)

set(SRC_FILES
)

target_sources(${PROJECT_NAME}
        PUBLIC FILE_SET thyme_ui TYPE CXX_MODULES FILES ${MODULE_FILES}
        PRIVATE ${SRC_FILES}
)