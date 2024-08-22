project(thyme VERSION 0.0.1)

set(header_path "${${PROJECT_NAME}_SOURCE_DIR}/include")
set(souce_path "${${PROJECT_NAME}_SOURCE_DIR}/src")

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

add_library(${PROJECT_NAME} SHARED "${header_path}/lib.hpp" "${souce_path}/lib.cpp" )

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 23)

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
    PRIVATE 
        "${header_path}/pch.hpp"
)

# if (BUILD_TESTING)
#     add_subdirectory("tests")
# endif()