export module th.render_system.vulkan:shader;

import std;

import glslang;

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

export enum struct ShaderLanguaType {
    glsl,
    hlsl
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

auto toEshSource(const ShaderLanguaType type) -> glslang::EShSource {
    switch (type) {
        case ShaderLanguaType::glsl: return glslang::EShSource::EShSourceGlsl;
        case ShaderLanguaType::hlsl: return glslang::EShSource::EShSourceHlsl;
    }
    std::unreachable();
}

export class VulkanShader {
public:
    VulkanShader(ShaderLanguaType language, const ShaderType type, const std::string& data) {
        initializeContext();
        const auto shader_stage = toGlslang(type);
        auto shader = glslang::TShader(shader_stage);
        const char* d = data.c_str();
        shader.setStrings(&d, 1);
        constexpr int client_input_semantic_version = 100;
        shader.setEnvInput(glslang::EShSource::EShSourceGlsl,
                           shader_stage,
                           glslang::EShClientVulkan,
                           client_input_semantic_version);
        shader.setEnvClient(glslang::EShClient::EShClientVulkan, glslang::EShTargetClientVersion::EShTargetVulkan_1_0);
        shader.setEnvTarget(glslang::EShTargetLanguage::EshTargetSpv,
                            glslang::EShTargetLanguageVersion::EShTargetSpv_1_4);
        constexpr auto message = static_cast<EShMessages>(EShMsgSpvRules | EShMsgDebugInfo | EShMsgSpvRules);
        if (constexpr int default_version = 100;
            !shader.parse(GetDefaultResources(), default_version, false, message)) {
            std::println("Can't parse shader: {}, {}", shader.getInfoLog(), shader.getInfoDebugLog());
        }

        glslang::TProgram program;
        program.addShader(&shader);
        if (!program.link(message)) {
            std::println("Can't link shader: {}, {}", program.getInfoLog(), program.getInfoDebugLog());
        }
        std::vector<uint32_t> spir_v;
        spv::SpvBuildLogger logger;
        glslang::SpvOptions spir_v_options{
            .generateDebugInfo = true,
            .emitNonSemanticShaderDebugInfo = true,
            .emitNonSemanticShaderDebugSource = true
        };

        glslang::GlslangToSpv(*program.getIntermediate(shader_stage), spir_v, &logger, &spir_v_options);
        m_spir_v_result.resize(spir_v.size());
        std::ranges::copy(spir_v, m_spir_v_result.begin());
    }

    [[nodiscard]] auto getSpirV() const -> const std::vector<uint32_t>& {
        return m_spir_v_result;
    }

private:
    static void initializeContext() {
        [[maybe_unused]] static GlslangContext glslang;
    }

    std::vector<uint32_t> m_spir_v_result;
};

}// namespace th
