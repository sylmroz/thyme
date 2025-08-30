module;

#include <imgui.h>

export module th.platform.imgui_context;

export namespace th {

class ImGuiContext {

public:
    explicit ImGuiContext() {
        init();
    }

    ~ImGuiContext() noexcept {
        ImGui::DestroyContext();
    }

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
