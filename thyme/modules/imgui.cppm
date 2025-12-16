module;

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

export module imgui;

export {
using ::ImGuiIO;
using ::ImVec2;
using ::ImVec4;
using ::ImVector;
using ::ImFont;
using ::ImGuiID;
using ::ImGuiViewport;
using ::ImGuiWindowFlags;


using ::ImGuiDockNodeFlags;
using ::ImGuiDockNodeFlags_::ImGuiDockNodeFlags_None;
using ::ImGuiDockNodeFlags_::ImGuiDockNodeFlags_KeepAliveOnly;
using ::ImGuiDockNodeFlags_::ImGuiDockNodeFlags_NoDockingOverCentralNode;
using ::ImGuiDockNodeFlags_::ImGuiDockNodeFlags_PassthruCentralNode;
using ::ImGuiDockNodeFlags_::ImGuiDockNodeFlags_NoDockingSplit;
using ::ImGuiDockNodeFlags_::ImGuiDockNodeFlags_NoResize;
using ::ImGuiDockNodeFlags_::ImGuiDockNodeFlags_AutoHideTabBar;
using ::ImGuiDockNodeFlags_::ImGuiDockNodeFlags_NoUndocking;

using ::ImGuiStyleVar;
using ::ImGuiStyleVar_WindowRounding;
using ::ImGuiStyleVar_WindowPadding;
using ::ImGuiStyleVar_WindowBorderSize;
using ::ImGuiStyleVar_WindowPadding;
using ::ImGuiStyleVar_ChildRounding;

using ::ImGuiWindowFlags_;
using ::ImGuiWindowFlags_None;
using ::ImGuiWindowFlags_NoTitleBar;
using ::ImGuiWindowFlags_NoResize;
using ::ImGuiWindowFlags_NoMove;
using ::ImGuiWindowFlags_NoScrollbar;
using ::ImGuiWindowFlags_NoScrollWithMouse;
using ::ImGuiWindowFlags_NoCollapse;
using ::ImGuiWindowFlags_AlwaysAutoResize;
using ::ImGuiWindowFlags_NoBackground;
using ::ImGuiWindowFlags_NoSavedSettings;
using ::ImGuiWindowFlags_NoMouseInputs;
using ::ImGuiWindowFlags_MenuBar;
using ::ImGuiWindowFlags_HorizontalScrollbar;
using ::ImGuiWindowFlags_NoFocusOnAppearing;
using ::ImGuiWindowFlags_NoBringToFrontOnFocus;
using ::ImGuiWindowFlags_AlwaysVerticalScrollbar;
using ::ImGuiWindowFlags_AlwaysHorizontalScrollbar;
using ::ImGuiWindowFlags_NoNavInputs;
using ::ImGuiWindowFlags_NoNavFocus;
using ::ImGuiWindowFlags_UnsavedDocument;
using ::ImGuiWindowFlags_NoNav;
using ::ImGuiWindowFlags_NoDecoration;
using ::ImGuiWindowFlags_NoInputs;
using ::ImGuiWindowFlags_ChildWindow;
using ::ImGuiWindowFlags_Tooltip;
using ::ImGuiWindowFlags_Popup;
using ::ImGuiWindowFlags_Modal;
using ::ImGuiWindowFlags_ChildMenu;


using ::ImGuiConfigFlags_::ImGuiConfigFlags_None;
using ::ImGuiConfigFlags_::ImGuiConfigFlags_NavEnableKeyboard;
using ::ImGuiConfigFlags_::ImGuiConfigFlags_NavEnableGamepad;
using ::ImGuiConfigFlags_::ImGuiConfigFlags_NoMouse;
using ::ImGuiConfigFlags_::ImGuiConfigFlags_NoMouseCursorChange;
using ::ImGuiConfigFlags_::ImGuiConfigFlags_NoKeyboard;
using ::ImGuiConfigFlags_::ImGuiConfigFlags_DockingEnable;
using ::ImGuiConfigFlags_::ImGuiConfigFlags_ViewportsEnable;
using ::ImGuiConfigFlags_::ImGuiConfigFlags_IsSRGB;
using ::ImGuiConfigFlags_::ImGuiConfigFlags_IsTouchScreen;

using ::ImGuiCol_::ImGuiCol_Text;
using ::ImGuiCol_::ImGuiCol_TextDisabled;
using ::ImGuiCol_::ImGuiCol_WindowBg;
using ::ImGuiCol_::ImGuiCol_ChildBg;
using ::ImGuiCol_::ImGuiCol_PopupBg;
using ::ImGuiCol_::ImGuiCol_Border;
using ::ImGuiCol_::ImGuiCol_BorderShadow;
using ::ImGuiCol_::ImGuiCol_FrameBg;
using ::ImGuiCol_::ImGuiCol_FrameBgHovered;
using ::ImGuiCol_::ImGuiCol_FrameBgActive;
using ::ImGuiCol_::ImGuiCol_TitleBg;
using ::ImGuiCol_::ImGuiCol_TitleBgActive;
using ::ImGuiCol_::ImGuiCol_TitleBgCollapsed;
using ::ImGuiCol_::ImGuiCol_MenuBarBg;
using ::ImGuiCol_::ImGuiCol_ScrollbarBg;
using ::ImGuiCol_::ImGuiCol_ScrollbarGrab;
using ::ImGuiCol_::ImGuiCol_ScrollbarGrabHovered;
using ::ImGuiCol_::ImGuiCol_ScrollbarGrabActive;
using ::ImGuiCol_::ImGuiCol_CheckMark;
using ::ImGuiCol_::ImGuiCol_SliderGrab;
using ::ImGuiCol_::ImGuiCol_SliderGrabActive;
using ::ImGuiCol_::ImGuiCol_Button;
using ::ImGuiCol_::ImGuiCol_ButtonHovered;
using ::ImGuiCol_::ImGuiCol_ButtonActive;
using ::ImGuiCol_::ImGuiCol_Header;
using ::ImGuiCol_::ImGuiCol_HeaderHovered;
using ::ImGuiCol_::ImGuiCol_HeaderActive;
using ::ImGuiCol_::ImGuiCol_Separator;
using ::ImGuiCol_::ImGuiCol_SeparatorHovered;
using ::ImGuiCol_::ImGuiCol_SeparatorActive;
using ::ImGuiCol_::ImGuiCol_ResizeGrip;
using ::ImGuiCol_::ImGuiCol_ResizeGripHovered;
using ::ImGuiCol_::ImGuiCol_ResizeGripActive;
using ::ImGuiCol_::ImGuiCol_InputTextCursor;
using ::ImGuiCol_::ImGuiCol_TabHovered;
using ::ImGuiCol_::ImGuiCol_Tab;
using ::ImGuiCol_::ImGuiCol_TabSelected;
using ::ImGuiCol_::ImGuiCol_TabSelectedOverline;
using ::ImGuiCol_::ImGuiCol_TabDimmed;
using ::ImGuiCol_::ImGuiCol_TabDimmedSelected;
using ::ImGuiCol_::ImGuiCol_TabDimmedSelectedOverline;
using ::ImGuiCol_::ImGuiCol_DockingPreview;
using ::ImGuiCol_::ImGuiCol_DockingEmptyBg;
using ::ImGuiCol_::ImGuiCol_PlotLines;
using ::ImGuiCol_::ImGuiCol_PlotLinesHovered;
using ::ImGuiCol_::ImGuiCol_PlotHistogram;
using ::ImGuiCol_::ImGuiCol_PlotHistogramHovered;
using ::ImGuiCol_::ImGuiCol_TableHeaderBg;
using ::ImGuiCol_::ImGuiCol_TableBorderStrong;
using ::ImGuiCol_::ImGuiCol_TableBorderLight;
using ::ImGuiCol_::ImGuiCol_TableRowBg;
using ::ImGuiCol_::ImGuiCol_TableRowBgAlt;
using ::ImGuiCol_::ImGuiCol_TextLink;
using ::ImGuiCol_::ImGuiCol_TextSelectedBg;
using ::ImGuiCol_::ImGuiCol_TreeLines;
using ::ImGuiCol_::ImGuiCol_DragDropTarget;
using ::ImGuiCol_::ImGuiCol_NavCursor;
using ::ImGuiCol_::ImGuiCol_NavWindowingHighlight;
using ::ImGuiCol_::ImGuiCol_NavWindowingDimBg;
using ::ImGuiCol_::ImGuiCol_ModalWindowDimBg;
using ::ImGuiCol_::ImGuiCol_COUNT;

// using ::ImGuiCol_;
// using ::ImGuiStyleVar_;
// using ::ImGuiButtonFlags_;


using ::ImGui_ImplGlfw_NewFrame;
using ::ImGui_ImplGlfw_InitForVulkan;
using ::ImGui_ImplVulkan_InitInfo;
using ::ImGui_ImplVulkan_Init;
using ::ImGui_ImplVulkan_NewFrame;
using ::ImGui_ImplVulkan_Shutdown;
using ::ImGui_ImplVulkan_RenderDrawData;
using ::ImGui_ImplGlfw_Shutdown;
}

export namespace ImGui {

using ::ImGui::AcceptDragDropPayload;
using ::ImGui::ArrowButton;
using ::ImGui::Begin;
using ::ImGui::End;
using ::ImGui::InputText;
using ::ImGui::Text;
using ::ImGui::CreateContext;
using ::ImGui::DestroyContext;
using ::ImGui::Render;
using ::ImGui::GetIO;
using ::ImGui::StyleColorsDark;
using ::ImGui::GetStyle;
using ::ImGui::NewFrame;
using ::ImGui::GetDrawData;
using ::ImGui::UpdatePlatformWindows;
using ::ImGui::RenderPlatformWindowsDefault;
using ::ImGui::GetMainViewport;
using ::ImGui::SetNextWindowPos;
using ::ImGui::SetNextWindowSize;
using ::ImGui::SetNextWindowViewport;
using ::ImGui::PushStyleVar;
using ::ImGui::GetID;
using ::ImGui::GetWindowPos;
using ::ImGui::DockSpace;
using ::ImGui::PopStyleVar;

using ::ImGui::ShowDemoWindow;

}// namespace ImGui
