cmake_minimum_required(VERSION 3.28)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

project(thyme_app VERSION 0.0.1)

enable_testing()
if(NOT DEFINED BUILD_TESTING)
  set(BUILD_TESTING ON)
endif()

add_subdirectory("thyme")
add_subdirectory("app")
