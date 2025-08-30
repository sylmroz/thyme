set(MODULE_FILES
        camera.cppm
        model.cppm
        transformation.cppm
        texture_data.cppm
)

set(SRC_FILES

)

target_sources(${PROJECT_NAME}
        PUBLIC FILE_SET thyme_scene TYPE CXX_MODULES FILES ${MODULE_FILES}
        PRIVATE ${SRC_FILES}
)