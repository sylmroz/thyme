#include <shaderc/shaderc.hpp>

#include <thyme/core/logger.hpp>

import thyme.platform.vulkan;

[[nodiscard]] auto compileGlslToSpv(const std::span<const char> source, const shaderc_shader_kind kind)
        -> std::vector<uint32_t> {
    shaderc::CompileOptions compileOptions;
    compileOptions.SetOptimizationLevel(shaderc_optimization_level_performance);
    const auto result = shaderc::Compiler{}.CompileGlslToSpv(source.data(), source.size(), kind, "main", {});
    if(const auto status = result.GetCompilationStatus(); status != shaderc_compilation_status_success) {
        const auto message = std::format("Failed to compile shader. Shaderrc status: {}", std::to_string(status));
        TH_API_LOG_ERROR(message);
        throw std::runtime_error(message);
    }
    return std::vector<uint32_t>{ result.cbegin(), result.cend() };
}