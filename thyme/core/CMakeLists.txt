target_sources(${PROJECT_NAME} PUBLIC FILE_SET thyme_core TYPE CXX_MODULES FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/application.cppm
        ${CMAKE_CURRENT_SOURCE_DIR}/engine.cppm
        ${CMAKE_CURRENT_SOURCE_DIR}/events.cppm
        ${CMAKE_CURRENT_SOURCE_DIR}/key_codes.cppm
        ${CMAKE_CURRENT_SOURCE_DIR}/logger.cppm
        ${CMAKE_CURRENT_SOURCE_DIR}/mouse_codes.cppm
        ${CMAKE_CURRENT_SOURCE_DIR}/utils.cppm
)
