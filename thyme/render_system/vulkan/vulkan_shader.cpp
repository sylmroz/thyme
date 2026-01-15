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
    shader.setEnvTarget(glslang::EShTargetLanguage::EshTargetSpv, glslang::EShTargetLanguageVersion::EShTargetSpv_1_4);
    constexpr auto message = static_cast<EShMessages>(EShMsgSpvRules | EShMsgDebugInfo | EShMsgSpvRules);
    if (constexpr int default_version = 100; !shader.parse(GetDefaultResources(), default_version, false, message)) {
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

void SlangShaderCompiler::compile(const std::string_view target) {
    Slang::ComPtr<slang::IGlobalSession> global_session;
    slang::createGlobalSession(global_session.writeRef());

    const auto target_desc = std::array{ slang::TargetDesc{ .format = slang_spirv,
                                                            .profile = global_session->findProfile("spirv_1_5") } };

    auto options =
            std::array{ slang::CompilerOptionEntry{ slang::CompilerOptionName::EmitSpirvDirectly,
                                                    { slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr } } };

    const auto session_desc = slang::SessionDesc{ .targets = target_desc.data(),
                                                  .targetCount = static_cast<SlangInt>(target_desc.size()),
                                                  .defaultMatrixLayoutMode = slang_matrix_layout_column_major,
                                                  .compilerOptionEntries = options.data(),
                                                  .compilerOptionEntryCount = static_cast<uint32_t>(options.size()) };

    Slang::ComPtr<slang::ISession> session;
    global_session->createSession(session_desc, session.writeRef());
    const auto module_path = getBasePath(ShaderLanguage::slang) / target;
    const auto module_path_str = std::format("{}.slang", module_path.string());
    const auto slang_module =
            Slang::ComPtr{ session->loadModuleFromSource(target.data(), module_path_str.c_str(), nullptr, nullptr) };
    slang_module->getTargetCode(0, m_spir_vv_code.writeRef());
}
auto compileSlangShader(const std::string_view shader_name) -> std::vector<uint32_t> {
    Slang::ComPtr<slang::IGlobalSession> global_session;
    slang::createGlobalSession(global_session.writeRef());

    const auto target_desc = std::array{ slang::TargetDesc{ .format = slang_spirv,
                                                            .profile = global_session->findProfile("spirv_1_5") } };

    auto options =
            std::array{ slang::CompilerOptionEntry{ slang::CompilerOptionName::EmitSpirvDirectly,
                                                    { slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr } } };

    const auto session_desc = slang::SessionDesc{ .targets = target_desc.data(),
                                                  .targetCount = static_cast<SlangInt>(target_desc.size()),
                                                  .defaultMatrixLayoutMode = slang_matrix_layout_column_major,
                                                  .compilerOptionEntries = options.data(),
                                                  .compilerOptionEntryCount = static_cast<uint32_t>(options.size()) };

    Slang::ComPtr<slang::ISession> session;
    global_session->createSession(session_desc, session.writeRef());
    const auto module_path = getBasePath(ShaderLanguage::slang) / shader_name;
    const auto module_path_str = std::format("{}.slang", module_path.string());
    const auto slang_module = Slang::ComPtr{ session->loadModuleFromSource(
            shader_name.data(), module_path_str.c_str(), nullptr, nullptr) };
    Slang::ComPtr<slang::IBlob> spirv_code;
    slang_module->getTargetCode(0, spirv_code.writeRef());
    const auto spriv_code_begin_ptr = static_cast<const uint32_t*>(spirv_code->getBufferPointer());
    return std::vector(spriv_code_begin_ptr, spriv_code_begin_ptr + (spirv_code->getBufferSize() / 4));
}

}// namespace th