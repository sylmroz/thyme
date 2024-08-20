project(sandbox VERSION 0.0.1)

set(header_path "${${PROJECT_NAME}_SOURCE_DIR}/include")

add_executable(${PROJECT_NAME} "src/main.cpp")

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 23)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${header_path})

target_link_libraries(${PROJECT_NAME} PUBLIC thyme)

if (WIN32)
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}> $<TARGET_FILE_DIR:${PROJECT_NAME}>
		COMMAND_EXPAND_LISTS)
endif()