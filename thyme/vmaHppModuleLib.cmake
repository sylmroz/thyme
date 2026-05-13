add_library(VulkanMemoryAllocator-HppModule)
add_library(VulkanMemoryAllocator-Hpp::VulkanMemoryAllocator-HppModule ALIAS VulkanMemoryAllocator-HppModule)

target_include_directories(VulkanMemoryAllocator-HppModule PUBLIC ${vmahpp_SOURCE_DIR})

find_package(VulkanMemoryAllocator CONFIG REQUIRED)

target_link_libraries(VulkanMemoryAllocator-HppModule
        PUBLIC
        unofficial::VulkanMemoryAllocator-Hpp::VulkanMemoryAllocator-Hpp
        Vulkan::cppm
        GPUOpen::VulkanMemoryAllocator
)

set_target_properties(VulkanMemoryAllocator-HppModule PROPERTIES CXX_STANDARD 23)

# Add MSVC-specific compiler options for proper C++ module support
if(MSVC)
    target_compile_options(VulkanMemoryAllocator-HppModule PRIVATE
            /std:c++latest      # Use latest C++ standard for better module support
            /permissive-        # Standards conformance mode
            /Zc:__cplusplus     # Enable correct __cplusplus macro
            /EHsc               # Enable C++ exception handling
            /Zc:preprocessor    # Use conforming preprocessor
            /translateInclude   # Automatically translate #include to import for standard library
    )
endif()

target_sources(VulkanMemoryAllocator-HppModule
        PUBLIC
        FILE_SET vma_hpp_modules TYPE CXX_MODULES
        BASE_DIRS ${vmahpp_SOURCE_DIR}
        FILES ${vmahpp_SOURCE_DIR}/vk_mem_alloc.cppm
)
