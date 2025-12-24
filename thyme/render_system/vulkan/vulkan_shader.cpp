module;

module th.render_system.vulkan;

namespace th {

auto ShaderCompiler::compile() const -> std::vector<uint32_t> {
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
}

}