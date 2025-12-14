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

auto toGlslang(const ShaderType type) -> EShLanguage {
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

auto toEshSource(const ShaderLanguage type) -> glslang::EShSource {
    return glslang::EShSource::EShSourceGlsl;
}

auto toVkShaderStageFlag(const ShaderType type) -> vk::ShaderStageFlagBits {
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
    const auto shader_language_folder = [shader_language] {
        if (shader_language == ShaderLanguage::glsl) {
            return "glsl";
        }
        return "slang";
    }();
    return std::filesystem::current_path() / "shaders" / "glsl";
}

export class ShaderCompiler {
public:
    explicit ShaderCompiler(const ShaderType type, const std::string& data) : m_type(toGlslang(type)), m_data(data) {
        initializeContext();
    }

    [[nodiscard]] auto compile() const -> std::vector<uint32_t> {
        auto shader = glslang::TShader(m_type);
        const char* d = m_data.c_str();
        shader.setStrings(&d, 1);
        constexpr int client_input_semantic_version = 100;
        shader.setEnvInput(
                glslang::EShSource::EShSourceGlsl, m_type, glslang::EShClientVulkan, client_input_semantic_version);
        shader.setEnvClient(glslang::EShClient::EShClientVulkan, glslang::EShTargetClientVersion::EShTargetVulkan_1_0);
        shader.setEnvTarget(glslang::EShTargetLanguage::EshTargetSpv,
                            glslang::EShTargetLanguageVersion::EShTargetSpv_1_4);
        constexpr auto message = static_cast<EShMessages>(EShMsgSpvRules | EShMsgDebugInfo | EShMsgSpvRules);
        if (constexpr int default_version = 100;
            !shader.parse(GetDefaultResources(), default_version, false, message)) {
            throw std::runtime_error(
                    std::format("Can't parse shader: {}, {}", shader.getInfoLog(), shader.getInfoDebugLog()));
        }

        glslang::TProgram program;
        program.addShader(&shader);
        if (!program.link(message)) {
            throw std::runtime_error(
                    std::format("Can't link shader: {}, {}", program.getInfoLog(), program.getInfoDebugLog()));
        }
        std::vector<uint32_t> spir_v;
        spv::SpvBuildLogger build_logger;
        glslang::SpvOptions spir_v_options{ .generateDebugInfo = true,
                                            .emitNonSemanticShaderDebugInfo = true,
                                            .emitNonSemanticShaderDebugSource = true };

        glslang::GlslangToSpv(*program.getIntermediate(m_type), spir_v, &build_logger, &spir_v_options);
        return spir_v;
    };

private:
    static void initializeContext() {
        [[maybe_unused]] static GlslangContext glslang;
    }

private:
    EShLanguage m_type;
    std::string m_data;
};

export auto createShaderModule(const ShaderType type, const std::string_view file_name, const vk::raii::Device& device, const Logger& logger) -> vk::raii::ShaderModule {
    try {
        const auto data = readFile<std::string>(getBasePath(ShaderLanguage::glsl) / file_name);
        const auto spir_v = ShaderCompiler(type, data).compile();
        return device.createShaderModule(
                vk::ShaderModuleCreateInfo{ .codeSize = spir_v.size() * 4, .pCode = spir_v.data() });
    } catch (const std::exception& e) {
        logger.error("Cannot create shader module, {}", e.what());
        throw;
    }

}

export class VulkanShader {
public:

    static auto create(const ShaderType type, const std::string_view file_name, const vk::raii::Device& device, const Logger& logger) -> VulkanShader {
        try {
            const auto data = readFile<std::string>(getBasePath(ShaderLanguage::glsl) / file_name);
            return VulkanShader(type, data, device, logger);
        } catch (const std::exception& e) {
            logger.error("Cannot create shader module, {}", e.what());
            throw;
        }

    }

    VulkanShader(const ShaderType type, const std::string& data, const vk::raii::Device& device, const Logger& logger) {
        try {
            const auto spir_v = ShaderCompiler(type, data).compile();
            m_shader_stage = toVkShaderStageFlag(type);
            m_shader_module = device.createShaderModule(
                    vk::ShaderModuleCreateInfo{ .codeSize = spir_v.size() * 4, .pCode = spir_v.data() });
        } catch (const std::exception& e) {
            logger.error("Cannot create shader module: {}", e.what());
            throw std::runtime_error("Cannot create shader module");
        }
    }

    [[nodiscard]] auto getShaderStage() const -> vk::PipelineShaderStageCreateInfo {
        return vk::PipelineShaderStageCreateInfo{
            .stage = m_shader_stage,
            .module = *m_shader_module,
            .pName = "main",
        };
    }

private:
    vk::ShaderStageFlagBits m_shader_stage;
    vk::raii::ShaderModule m_shader_module{nullptr};
};

}// namespace th
