set(MODULE_FILES
        application.cppm
        engine.cppm
        events.cppm
        key_codes.cppm
        logger.cppm
        mouse_codes.cppm
        utils.cppm
)

set(SRC_FILES
        application.cpp
        engine.cpp
)

target_sources(${PROJECT_NAME}
        PUBLIC FILE_SET thyme_core TYPE CXX_MODULES FILES ${MODULE_FILES}
        PRIVATE ${SRC_FILES}
)
