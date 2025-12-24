export module th.render_system.vulkan:shader;

import std;

import glslang;
import vulkan;

import th.core.logger;
import th.core.utils;

class GlslangContext {
public:
    GlslangContext() {
        init();
    }
    ~GlslangContext() {
        finalize();
    }

private:
    static void init() noexcept {
        glslang::InitializeProcess();
    }

    static void finalize() noexcept {
        glslang::FinalizeProcess();
    }
};

namespace th {

export enum struct ShaderLanguage {
    glsl,
    slang
};

export enum struct ShaderType {
    vertex,
    fragment,
    geometry,
    tessellation_evaluation,
    tessellation_control,
    compute,
    ray_gen,
    ray_intersect,
    ray_any_hit,
    ray_closest_hit,
    ray_miss,
    callable,
    task,
    mesh
};

constexpr auto toGlslang(const ShaderType type) -> EShLanguage {
    switch (type) {
        case ShaderType::vertex: return EShLangVertex;
        case ShaderType::fragment: return EShLangFragment;
        case ShaderType::geometry: return EShLangGeometry;
        case ShaderType::tessellation_control: return EShLangTessControl;
        case ShaderType::tessellation_evaluation: return EShLangTessEvaluation;
        case ShaderType::compute: return EShLangCompute;
        case ShaderType::ray_gen: return EShLangRayGen;
        case ShaderType::ray_intersect: return EShLangRayGen;
        case ShaderType::ray_any_hit: return EShLangAnyHit;
        case ShaderType::ray_closest_hit: return EShLangClosestHit;
        case ShaderType::ray_miss: return EShLangMiss;
        case ShaderType::callable: return EShLangCallable;
        case ShaderType::task: return EShLangTask;
        case ShaderType::mesh: return EShLangMesh;
    }
    std::unreachable();
}

constexpr auto toEshSource(const ShaderLanguage type) -> glslang::EShSource {
    return glslang::EShSource::EShSourceGlsl;
}

constexpr auto toVkShaderStageFlag(const ShaderType type) -> vk::ShaderStageFlagBits {
    switch (type) {
        case ShaderType::vertex: return vk::ShaderStageFlagBits::eVertex;
        case ShaderType::fragment: return vk::ShaderStageFlagBits::eFragment;
        case ShaderType::geometry: return vk::ShaderStageFlagBits::eGeometry;
        case ShaderType::tessellation_control: return vk::ShaderStageFlagBits::eTessellationControl;
        case ShaderType::tessellation_evaluation: return vk::ShaderStageFlagBits::eTessellationEvaluation;
        case ShaderType::compute: return vk::ShaderStageFlagBits::eCompute;
        case ShaderType::ray_gen: return vk::ShaderStageFlagBits::eRaygenKHR;
        case ShaderType::ray_intersect: return vk::ShaderStageFlagBits::eIntersectionKHR;
        case ShaderType::ray_any_hit: return vk::ShaderStageFlagBits::eAnyHitKHR;
        case ShaderType::ray_closest_hit: return vk::ShaderStageFlagBits::eClosestHitKHR;
        case ShaderType::ray_miss: return vk::ShaderStageFlagBits::eMissKHR;
        case ShaderType::callable: return vk::ShaderStageFlagBits::eCallableKHR;
        case ShaderType::task: return vk::ShaderStageFlagBits::eTaskNV;
        case ShaderType::mesh: return vk::ShaderStageFlagBits::eMeshNV;
    }
    std::unreachable();
}

auto getBasePath(const ShaderLanguage shader_language) -> std::filesystem::path {
    const auto shader_language_folder = [shader_language] -> std::string {
        if (shader_language == ShaderLanguage::glsl) {
            return "glsl";
        }
        return "slang";
    }();
    return std::filesystem::current_path() / "shaders" / shader_language_folder;
}

export class ShaderCompiler {
public:
    explicit ShaderCompiler(const ShaderType type, const std::string& data) : m_type(toGlslang(type)), m_data(data) {
        initializeContext();
    }

    [[nodiscard]] auto compile() const -> std::vector<uint32_t>;

private:
    static void initializeContext() {
        [[maybe_unused]] static GlslangContext glslang;
    }

private:
    EShLanguage m_type;
    std::string m_data;
};


export auto createShaderModule(const vk::raii::Device& device, const std::span<const uint32_t> spir_v,
                               const Logger& logger) -> vk::raii::ShaderModule {
    try {
        return device.createShaderModule(
                vk::ShaderModuleCreateInfo{ .codeSize = spir_v.size() * 4, .pCode = spir_v.data() });
    } catch (const std::exception& e) {
        logger.error("Cannot create shader module, {}", e.what());
        throw;
    }
}


export template <typename Compiler>
auto createShaderModule(const vk::raii::Device& device, const Compiler& compiler, const Logger& logger)
        -> vk::raii::ShaderModule {
    try {
        const auto spir_v = compiler.compile();
        return createShaderModule(device, std::span(spir_v), logger);
    } catch (const std::exception& e) {
        logger.error("Cannot create shader module, {}", e.what());
        throw;
    }
}

export auto createShaderModule(const ShaderType type, const std::string_view file_name, const vk::raii::Device& device,
                               const Logger& logger) -> vk::raii::ShaderModule {
    try {
        const auto data = readFile<std::string>(getBasePath(ShaderLanguage::glsl) / file_name);
        return createShaderModule(device, ShaderCompiler(type, data), logger);
        const auto spir_v = ShaderCompiler(type, data).compile();
    } catch (const std::exception& e) {
        logger.error("Cannot create shader module, {}", e.what());
        throw;
    }
}

export class VulkanShader {
public:
    static auto create(const ShaderType type, const std::string_view file_name, const vk::raii::Device& device,
                       const Logger& logger) -> VulkanShader {
        try {
            auto shader = createShaderModule(type, file_name, device, logger);
            return VulkanShader(toVkShaderStageFlag(type), std::move(shader));
        } catch (const std::exception& e) {
            logger.error("Cannot create shader module, {}", e.what());
            throw;
        }
    }

    VulkanShader(vk::ShaderStageFlagBits shader_stage_flag_bits, vk::raii::ShaderModule shader_module,
                 const std::string_view entry_point = "main")
        : m_shader_stage(std::move(shader_stage_flag_bits)), m_shader_module(std::move(shader_module)),
          m_entry_point(entry_point) {}

    [[nodiscard]] auto getShaderStage() const -> vk::PipelineShaderStageCreateInfo {
        return vk::PipelineShaderStageCreateInfo{
            .stage = m_shader_stage,
            .module = *m_shader_module,
            .pName = m_entry_point.c_str(),
        };
    }

private:
    vk::ShaderStageFlagBits m_shader_stage;
    vk::raii::ShaderModule m_shader_module;
    std::string m_entry_point;
};

}// namespace th
