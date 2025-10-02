project(app VERSION 0.0.1)

set(header_path "${${PROJECT_NAME}_SOURCE_DIR}/include")

add_executable(${PROJECT_NAME} "src/main.cpp")

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${header_path})

find_package(spdlog REQUIRED)

target_link_libraries(${PROJECT_NAME}
	PUBLIC
		thyme
	PRIVATE 
		spdlog::spdlog_header_only
)

if (WIN32)
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}> $<TARGET_FILE_DIR:${PROJECT_NAME}>
		COMMAND_EXPAND_LISTS)
endif()

message(STATUS "copy file from ${CMAKE_SOURCE_DIR}/app/assets to  ${CMAKE_CURRENT_BINARY_DIR}/assets")

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different ${CMAKE_SOURCE_DIR}/app/assets ${CMAKE_CURRENT_BINARY_DIR}/assets
		COMMENT "Copying app assets folder"
)