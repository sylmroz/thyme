project(thyme VERSION 0.0.1)

set(header_path "${${PROJECT_NAME}_SOURCE_DIR}/include/thyme")
set(source_path "${${PROJECT_NAME}_SOURCE_DIR}/src")

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

add_library(${PROJECT_NAME})

target_sources(${PROJECT_NAME}
	PRIVATE
		src/core/application.cpp
		src/core/engine.cpp

		src/platform/glfw_window.cpp

		src/platform/vulkan/graphic_pipeline.cpp
		src/platform/vulkan/renderer.cpp
		#src/platform/vulkan/shader.cpp #for now there are problems with integrating shaderc from google
		src/platform/vulkan/utils.cpp
	PUBLIC
		FILE_SET CXX_MODULES
		FILES
			include/thyme/core/application.cppm
			include/thyme/core/common_structs.cppm
			include/thyme/core/engine.cppm
			include/thyme/core/event.cppm
			include/thyme/core/key_codes.cppm
			include/thyme/core/layer.cppm
			include/thyme/core/layer_stack.cppm
			include/thyme/core/mouse_codes.cppm
			include/thyme/core/platform_context.cppm
			include/thyme/core/renderer.cppm
			include/thyme/core/utils.cppm
			include/thyme/core/window.cppm

			include/thyme/platform/glfw_vulkan_platform_context.cppm
			include/thyme/platform/glfw_window.cppm

			include/thyme/platform/vulkan/graphic_pipeline.cppm
			include/thyme/platform/vulkan/renderer.cppm
			#include/thyme/platform/vulkan/shader.cppm #for now there are problems with integrating shaderc from google
			include/thyme/platform/vulkan/utils.cppm
			include/thyme/platform/vulkan/vulkan.cppm
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 23)

configure_file("${header_path}/version.hpp.in" "${header_path}/version.hpp")

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
         $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

include(GenerateExportHeader)
generate_export_header(
    ${PROJECT_NAME} EXPORT_MACRO_NAME THYME_API BASE_NAME ${PROJECT_NAME} EXPORT_FILE_NAME
    ${CMAKE_CURRENT_BINARY_DIR}/include/${PROJECT_NAME}/export_macros.hpp)

target_precompile_headers(${PROJECT_NAME} 
    PUBLIC
        #"${header_path}/pch.hpp"
    PRIVATE 
        <array>
		<functional>
		<filesystem>
		<fstream>
        <iostream>
        <ranges>
        <map>
        <memory>
		<ostream>
        <set>
        <string>
        <string_view>
		<utility>
        <unordered_map>
		<variant>
        <vector>
        <type_traits>
)

find_package(spdlog REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(Vulkan REQUIRED COMPONENTS shaderc_combined)

target_link_libraries(${PROJECT_NAME} 
    PRIVATE
        spdlog::spdlog_header_only
        glfw
        Vulkan::Vulkan
        Vulkan::Headers
		Vulkan::shaderc_combined
)

# copy dlls to the app
if (WIN32)
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:app> $<TARGET_FILE_DIR:app>
		COMMAND_EXPAND_LISTS)
endif()

# if (BUILD_TESTING)
#     add_subdirectory("tests")
# endif()