module;

#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>

export module glslang;

export namespace glslang {
using ::glslang::InitializeProcess;
using ::glslang::FinalizeProcess;

using ::glslang::TShader;

using ::glslang::EShSource;
using ::glslang::EShSource::EShSourceGlsl;
using ::glslang::EShSource::EShSourceHlsl;

using ::glslang::EShClient;
using ::glslang::EShClientOpenGL;
using ::glslang::EShClientVulkan;

using ::glslang::EShTargetLanguage;
using ::glslang::EshTargetSpv;

using ::glslang::EShTargetClientVersion;
using ::glslang::EShTargetVulkan_1_0;
using ::glslang::EShTargetVulkan_1_1;
using ::glslang::EShTargetVulkan_1_2;
using ::glslang::EShTargetVulkan_1_3;
using ::glslang::EShTargetVulkan_1_4;

using ::glslang::EShTargetLanguageVersion;
using ::glslang::EShTargetSpv_1_0;
using ::glslang::EShTargetSpv_1_1;
using ::glslang::EShTargetSpv_1_2;
using ::glslang::EShTargetSpv_1_3;
using ::glslang::EShTargetSpv_1_4;
using ::glslang::EShTargetSpv_1_5;
using ::glslang::EShTargetSpv_1_6;

using ::glslang::TProgram;

using ::glslang::GlslangToSpv;

using ::glslang::SpvOptions;

}// namespace glslang

export namespace spv {
using ::spv::SpvBuildLogger;
}

export {
using ::EShLanguage;
using ::EShLanguage::EShLangVertex;
using ::EShLanguage::EShLangTessControl;
using ::EShLanguage::EShLangTessEvaluation;
using ::EShLanguage::EShLangGeometry;
using ::EShLanguage::EShLangFragment;
using ::EShLanguage::EShLangCompute;
using ::EShLanguage::EShLangRayGen;
using ::EShLanguage::EShLangRayGenNV;
using ::EShLanguage::EShLangIntersect;
using ::EShLanguage::EShLangIntersectNV;
using ::EShLanguage::EShLangAnyHit;
using ::EShLanguage::EShLangAnyHitNV;
using ::EShLanguage::EShLangClosestHit;
using ::EShLanguage::EShLangClosestHitNV;
using ::EShLanguage::EShLangMiss;
using ::EShLanguage::EShLangMissNV;
using ::EShLanguage::EShLangCallable;
using ::EShLanguage::EShLangCallableNV;
using ::EShLanguage::EShLangTask;
using ::EShLanguage::EShLangTaskNV;
using ::EShLanguage::EShLangMesh;
using ::EShLanguage::EShLangMeshNV;

using ::EShMessages;
using ::EShMsgSpvRules;
using ::EShMsgVulkanRules;
using ::EShMsgDebugInfo;

using ::GetDefaultResources;
}
