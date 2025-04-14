project(thyme VERSION 0.0.1)

set(header_path "${${PROJECT_NAME}_SOURCE_DIR}/include/thyme")
set(source_path "${${PROJECT_NAME}_SOURCE_DIR}/src")

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

add_library(${PROJECT_NAME})

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	# using Clang
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	# using GCC
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
	# using Intel C++
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	target_compile_options(${PROJECT_NAME} PRIVATE
			$<$<CONFIG:Debug>:/Wall /EHsc /ZI /MP>
			$<$<CONFIG:Release>:/GL /MP /O3>
			$<$<CONFIG:RelWithDebInfo>:>
			$<$<CONFIG:MinSizeRel>:>
	)
	target_link_options(${PROJECT_NAME} PUBLIC $<$<CONFIG:Debug>:/INCREMENTAL>)
endif()

target_sources(${PROJECT_NAME} PRIVATE
		src/core/application.cpp
		src/core/engine.cpp
		src/core/utils.cpp
		src/scene/transformation.cpp
		src/scene/model.cpp
		src/scene/texture.cpp

		src/platform/glfw_window.cpp

		src/platform/vulkan/graphic_pipeline.cpp
		src/platform/vulkan/gui.cpp
		src/platform/vulkan/model.cpp
		src/platform/vulkan/renderer.cpp
		#src/platform/vulkan/shader.cpp #for now there are problems with integrating shaderc from google
		src/platform/vulkan/swapchain.cpp
		src/platform/vulkan/texture.cpp
		src/platform/vulkan/utils.cpp
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 23)

configure_file("${header_path}/version.hpp.in" "${header_path}/version.hpp")

target_include_directories(${PROJECT_NAME}
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
		$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
	PRIVATE
		${Stb_INCLUDE_DIR}
)

include(GenerateExportHeader)
generate_export_header(
    ${PROJECT_NAME} EXPORT_MACRO_NAME THYME_API BASE_NAME ${PROJECT_NAME} EXPORT_FILE_NAME
    ${CMAKE_CURRENT_BINARY_DIR}/include/${PROJECT_NAME}/export_macros.hpp)

target_precompile_headers(${PROJECT_NAME} 
    PUBLIC
        #"${header_path}/pch.hpp"
    PRIVATE
		### std
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
		#### glm
		<glm/glm.hpp>
)

find_package(spdlog REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(Stb REQUIRED)

target_link_libraries(${PROJECT_NAME} 
    PRIVATE
        spdlog::spdlog_header_only
        glfw
        Vulkan::Vulkan
        Vulkan::Headers
		imgui::imgui
)

# copy dlls to the app
if (WIN32)
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:app> $<TARGET_FILE_DIR:app>
		COMMAND_EXPAND_LISTS)
endif()

#copy ifc files for IntelliSense
install (
	DIRECTORY $(CMAKE_CURRENT_BINARY_DIR)/CMakeFiles/thyme.dir
	DESTINATION $(CMAKE_CURRENT_BINARY_DIR)/ifc
	FILES_MATCHING PATTERN "*.ifc"
)

# if (BUILD_TESTING)
#     add_subdirectory("tests")
# endif()