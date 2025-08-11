target_sources(${PROJECT_NAME} PUBLIC FILE_SET thyme_core TYPE CXX_MODULES FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/logger.cppm
        ${CMAKE_CURRENT_SOURCE_DIR}/utils.cppm
)
