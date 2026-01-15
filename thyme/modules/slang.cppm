module;

#include <slang-com-helper.h>
#include <slang-com-ptr.h>
#include <slang.h>

export module slang;

export {
constexpr auto slang_spirv = SLANG_SPIRV;
constexpr auto slang_matrix_layout_column_major = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR;
using ::SlangInt;
}

export namespace Slang {
using ::Slang::ComPtr;
}

export namespace slang {
using ::slang::IGlobalSession;
using ::slang::SessionDesc;
using ::slang::TargetDesc;
using ::slang::ISession;
using ::slang::IModule;
using ::slang::IEntryPoint;
using ::slang::IComponentType;
using ::slang::IBlob;

using ::slang::CompilerOptionEntry;
using ::slang::CompilerOptionName;
using ::slang::CompilerOptionValueKind;

using ::slang::createGlobalSession;
}// namespace slang
