add_library(unofficial::VulkanMemoryAllocator-Hpp::VulkanMemoryAllocator-Hpp INTERFACE IMPORTED)

set_target_properties(
	unofficial::VulkanMemoryAllocator-Hpp::VulkanMemoryAllocator-Hpp
	PROPERTIES
		INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_LIST_DIR}/../../include"
)

include(${CMAKE_ROOT}/Modules/FindPackageHandleStandardArgs.cmake)
include(${CMAKE_ROOT}/Modules/SelectLibraryConfigurations.cmake)

if(NOT vmahpp_SOURCE_DIR)
	find_path(vmahpp_SOURCE_DIR NAMES vk_mem_alloc.cppm PATHS ${vulkan-memory-allocator-hpp_DIR} PATH_SUFFIXES include/vulkan-memory-allocator-hpp)
endif()

find_package_handle_standard_args(ulkan-memory-allocator-hpp DEFAULT_MSG vmahpp_SOURCE_DIR)
mark_as_advanced(vmahpp_SOURCE_DIR)
