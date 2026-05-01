SET(MODULE_FILES
        render_graph.cppm
        renderer.cppm
)

set(SRC_FILES
        renderer.cpp
)

target_sources(${PROJECT_NAME}
        PUBLIC FILE_SET thyme_render_system TYPE CXX_MODULES FILES ${MODULE_FILES}
        PRIVATE ${SRC_FILES}
)

add_subdirectory(vulkan)
add_subdirectory(passes)