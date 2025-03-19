#pragma once

#include <imgui.h>

#include <thyme/core/platform_context.hpp>

namespace th {

class ImGuiContext: public PlatformContext {
public:
    explicit ImGuiContext()
        : PlatformContext(PlatformContextArguments{ .initializer = [] { init(); },
                                                    .destroyer = [] { ImGui::DestroyContext(); } }) {}

private:
    static void init() noexcept {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        ImGui::StyleColorsDark();

        auto& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }
    }
};
}// namespace th