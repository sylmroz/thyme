add_library(glmCppModule)
add_library(glm::cppm ALIAS glmCppModule)

target_compile_definitions(glmCppModule
        PUBLIC
        GLM_FORCE_RADIANS
        GLM_FORCE_DEPTH_ZERO_TO_ONE
        GLM_GTX_INLINE_NAMESPACE
        GLM_ENABLE_EXPERIMENTAL
)
target_include_directories(glmCppModule
        PUBLIC
        "${Stb_INCLUDE_DIR}/glm"
)
target_link_libraries(glmCppModule
        PUBLIC
        glm::glm
)

set_target_properties(glmCppModule PROPERTIES CXX_STANDARD 23)

# Add MSVC-specific compiler options for proper C++ module support
if(MSVC)
    target_compile_options(glmCppModule PRIVATE
            /std:c++latest      # Use latest C++ standard for better module support
            /permissive-        # Standards conformance mode
            /Zc:__cplusplus     # Enable correct __cplusplus macro
            /EHsc               # Enable C++ exception handling
            /Zc:preprocessor    # Use conforming preprocessor
            /translateInclude   # Automatically translate #include to import for standard library
    )
endif()



target_sources(glmCppModule
        PUBLIC
        FILE_SET glm_modules TYPE CXX_MODULES
        BASE_DIRS ${Stb_INCLUDE_DIR}/glm
        FILES ${Stb_INCLUDE_DIR}/glm/glm.cppm
)

# Add the vulkan.cppm file directly as a source file
target_sources(glmCppModule
        PRIVATE
        ${Stb_INCLUDE_DIR}/glm/glm.cppm
)