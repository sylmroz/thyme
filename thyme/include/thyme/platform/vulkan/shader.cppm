module;

#include <shaderc/shaderc.hpp>

export module thyme.platform.vulkan:shader;

namespace Thyme::Vulkan {
export enum class ShaderType {
    vertex,
    fragment,
    geometry,
    tess_control,
    tess_evaluation,
    computation
};

static std::map<ShaderType, shaderc_shader_kind> shaderTypeToKind = {
    { ShaderType::vertex, shaderc_vertex_shader },
    { ShaderType::fragment, shaderc_fragment_shader },
    { ShaderType::geometry, shaderc_geometry_shader },
    { ShaderType::tess_control, shaderc_tess_control_shader },
    { ShaderType::tess_evaluation, shaderc_tess_evaluation_shader },
};

export [[nodiscard]] auto compileGlslToSpv(const std::span<const char> source, const shaderc_shader_kind kind)
        -> std::vector<uint32_t>;

export [[nodiscard]] auto compileGlslToSpv(const std::span<const char> source, const ShaderType type)
        -> std::vector<uint32_t> {
    return compileGlslToSpv(source, shaderTypeToKind[type]);
}
}// namespace Thyme::Vulkan