set(MODULE_FILES
        glfw.cppm
        json.cppm
)

SET(SOURCE_FILES
)

target_sources(${PROJECT_NAME}
        PUBLIC FILE_SET thyme_modules TYPE CXX_MODULES FILES ${MODULE_FILES}
        PRIVATE ${SOURCE_FILES}
)
