set(MODULE_FILES
        glfw.cppm
        glslang.cppm
        imgui.cppm
        json.cppm
        slang.cppm
        stb_image.cppm
)

if (NOT LOGGER_USE_STD_PRINT)
    set (MODULE_FILES ${MODULE_FILES} spdlog.cppm)
endif ()

SET(SOURCE_FILES
)

target_sources(${PROJECT_NAME}
        PUBLIC FILE_SET thyme_modules TYPE CXX_MODULES FILES ${MODULE_FILES}
        PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${Stb_INCLUDE_DIR/glm} FILES ${Stb_INCLUDE_DIR}/glm/glm.cppm
        PRIVATE ${SOURCE_FILES}
)
