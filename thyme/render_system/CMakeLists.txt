cmake_minimum_required(VERSION 3.28)

target_sources(${PROJECT_NAME} PUBLIC FILE_SET thyme_render_target TYPE CXX_MODULES FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/device.cppm
        ${CMAKE_CURRENT_SOURCE_DIR}/framework.cppm
        ${CMAKE_CURRENT_SOURCE_DIR}/framework_factory.cppm
        ${CMAKE_CURRENT_SOURCE_DIR}/swap_chain.cppm
)

add_subdirectory(vulkan)

#target_include_directories(${PROJECT_NAME}} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})