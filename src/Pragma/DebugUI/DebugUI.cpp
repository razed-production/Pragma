#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include "Pragma/DebugUI/DebugUI.h"

#include "Pragma/Core/DemoScene.h"
#include "Pragma/Core/Log.h"
#include "Pragma/Core/Profiler.h"
#include "Pragma/Editor/EditorUI.h"
#include "Pragma/Physics/PhysicsSystem.h"
#include "Pragma/RHI/DX11/DX11Device.h"
#include "Pragma/RHI/Device.h"
#include "Pragma/Scripting/ManagedScriptHost.h"
#include "imgui.h"
#include "backends/imgui_impl_dx11.h"
#include "backends/imgui_impl_win32.h"

#include <stdexcept>
#include <filesystem>
#include <string>

#include <windows.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Pragma::DebugUI
{
namespace
{
std::filesystem::path GetEditorSavedDirectory()
{
    wchar_t buffer[MAX_PATH]{};
    const DWORD length = GetModuleFileNameW(nullptr, buffer, static_cast<DWORD>(std::size(buffer)));
    std::filesystem::path directory = std::filesystem::path(std::wstring(buffer, length)).parent_path().parent_path().parent_path() / "saved";
    std::filesystem::create_directories(directory);
    return directory;
}
}

DebugUI::DebugUI()
    : m_editorUi(std::make_unique<Pragma::Editor::EditorUI>())
{
}

DebugUI::~DebugUI()
{
    Shutdown();
}

void DebugUI::Initialize(Pragma::RHI::IDevice& device, const Pragma::RHI::NativeWindow window)
{
    if (m_initialized)
    {
        return;
    }

    auto* dx11Device = dynamic_cast<Pragma::RHI::DX11::DX11Device*>(&device);
    if (dx11Device == nullptr)
    {
        throw std::runtime_error("DebugUI currently supports only the DX11 backend.");
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    m_imguiIniPath = (GetEditorSavedDirectory() / "imgui_layout.ini").string();
    io.IniFilename = m_imguiIniPath.c_str();

    ImGui::StyleColorsDark();

    if (!ImGui_ImplWin32_Init(window.Handle))
    {
        throw std::runtime_error("Failed to initialize ImGui Win32 backend.");
    }

    if (!ImGui_ImplDX11_Init(dx11Device->GetNativeDevice(), dx11Device->GetImmediateContext()))
    {
        ImGui_ImplWin32_Shutdown();
        throw std::runtime_error("Failed to initialize ImGui DX11 backend.");
    }

    m_initialized = true;
    Pragma::Core::Log(Pragma::Core::LogCategory::DebugUI, Pragma::Core::LogLevel::Info, "Dear ImGui initialized.");
}

void DebugUI::Shutdown()
{
    if (!m_initialized)
    {
        return;
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    m_initialized = false;
}

bool DebugUI::HandleWindowMessage(void* hwnd, const unsigned int message, const std::size_t wParam, const std::ptrdiff_t lParam)
{
    if (!m_initialized)
    {
        return false;
    }

    return ImGui_ImplWin32_WndProcHandler(
        static_cast<HWND>(hwnd),
        message,
        static_cast<WPARAM>(wParam),
        static_cast<LPARAM>(lParam)) != 0;
}

void DebugUI::BeginFrame(
    const float deltaSeconds,
    Pragma::Core::DemoScene& demoScene,
    Pragma::Physics::PhysicsSystem& physicsSystem,
    Pragma::Scripting::ManagedScriptHost& managedScriptHost)
{
    if (!m_initialized)
    {
        return;
    }

    static bool hasLoggedFirstUiFrame = false;
    if (!hasLoggedFirstUiFrame)
    {
        hasLoggedFirstUiFrame = true;
        Pragma::Core::Log(Pragma::Core::LogCategory::DebugUI, Pragma::Core::LogLevel::Info, "Dear ImGui first frame submitted.");
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    m_editorUi->BeginFrame(
        deltaSeconds,
        demoScene,
        physicsSystem,
        m_showDiagnosticsWindow,
        m_showProfilerWindow,
        m_showLogConsoleWindow);

    Pragma::Renderer::Scene& scene = demoScene.GetScene();
    const Pragma::Renderer::SceneObject* activeCameraObject = scene.GetActiveCameraObject();
    const bool hasActiveCamera = activeCameraObject != nullptr && activeCameraObject->HasCamera();
    int lightCount = 0;
    int physicsIssueCount = 0;
    for (const Pragma::Renderer::SceneObject& object : scene.GetObjects())
    {
        if (object.HasLight())
        {
            ++lightCount;
        }

        if (object.HasRigidBody() != object.HasBoxCollider())
        {
            ++physicsIssueCount;
        }
        else if (const auto* boxCollider = object.GetBoxCollider(); boxCollider != nullptr)
        {
            if (boxCollider->HalfExtent.X <= 0.0f || boxCollider->HalfExtent.Y <= 0.0f || boxCollider->HalfExtent.Z <= 0.0f)
            {
                ++physicsIssueCount;
            }
        }
    }
    const std::size_t physicsBodyCount = physicsSystem.GetBodyCount();
    const std::size_t activePhysicsBodyCount = physicsSystem.GetActiveBodyCount();

    const std::vector<Pragma::Core::LogEntry> logEntries = Pragma::Core::GetLogEntriesSnapshot();
    const Pragma::Core::FrameProfile frameProfile = Pragma::Core::GetLastFrameProfileSnapshot();
    const Pragma::Core::GpuFrameProfile gpuFrameProfile = Pragma::Core::GetLastGpuFrameProfileSnapshot();
    const double lastFrameMilliseconds = Pragma::Core::GetLastFrameMilliseconds();
    const double peakFrameMilliseconds = Pragma::Core::GetPeakFrameMilliseconds();
    const std::uint64_t peakFrameIndex = Pragma::Core::GetPeakFrameIndex();
    const double lastGpuFrameMilliseconds = Pragma::Core::GetLastGpuFrameMilliseconds();
    const double peakGpuFrameMilliseconds = Pragma::Core::GetPeakGpuFrameMilliseconds();
    const std::uint64_t peakGpuFrameIndex = Pragma::Core::GetPeakGpuFrameIndex();

    int warningCount = 0;
    int errorCount = 0;
    for (const Pragma::Core::LogEntry& entry : logEntries)
    {
        if (entry.Level == Pragma::Core::LogLevel::Warning)
        {
            ++warningCount;
        }
        else if (entry.Level == Pragma::Core::LogLevel::Error)
        {
            ++errorCount;
        }
    }

    const Pragma::Renderer::SceneObject* selectedObject = scene.FindObject(m_editorUi->GetSelectedObjectId());

    if (m_showDiagnosticsWindow)
    {
        ImGui::SetNextWindowSize(ImVec2(360.0f, 140.0f), ImGuiCond_FirstUseEver);
        ImGui::Begin("Diagnostics", &m_showDiagnosticsWindow);
        ImGui::Text("Warnings: %d", warningCount);
        ImGui::Text("Errors: %d", errorCount);
        ImGui::Separator();
        ImGui::Text("Scene Objects: %s", scene.GetObjects().empty() ? "Missing" : "OK");
        ImGui::Text("Directional Light: %s", lightCount > 0 ? "OK" : "Missing");
        ImGui::Text("Active Camera: %s", hasActiveCamera ? "OK" : "Missing");
        ImGui::Text("Physics Bodies: %llu (%llu active)", static_cast<unsigned long long>(physicsBodyCount), static_cast<unsigned long long>(activePhysicsBodyCount));
        ImGui::Text("Physics Issues: %d", physicsIssueCount);
        ImGui::Text("Managed C# Projects: %llu", static_cast<unsigned long long>(managedScriptHost.GetProjectCount()));
        ImGui::Text("Managed Runtime Ready: %llu", static_cast<unsigned long long>(managedScriptHost.GetRuntimeReadyProjectCount()));
        ImGui::Text("Managed Entry Points Ready: %llu", static_cast<unsigned long long>(managedScriptHost.GetEntryPointReadyProjectCount()));
        ImGui::Text("Managed Binding Probes Ready: %llu", static_cast<unsigned long long>(managedScriptHost.GetBindingReadyProjectCount()));
        ImGui::Text("hostfxr: %s", managedScriptHost.IsHostFxrAvailable() ? "Ready" : "Unavailable");
        if (!managedScriptHost.GetHostFxrPath().empty())
        {
            ImGui::TextWrapped("hostfxr Path: %s", managedScriptHost.GetHostFxrPath().string().c_str());
        }
        ImGui::TextWrapped("%s", managedScriptHost.GetHostFxrStatus().c_str());
        ImGui::Text("Selected Object: %s", selectedObject != nullptr ? selectedObject->Name.c_str() : "None");
        ImGui::End();
    }

    if (m_showProfilerWindow)
    {
        ImGui::Begin("Profiler", &m_showProfilerWindow);
        ImGui::TextUnformatted("CPU");
        ImGui::Text("Last Frame: %.3f ms", lastFrameMilliseconds);
        ImGui::Text("Peak Frame: %.3f ms (frame %llu)", peakFrameMilliseconds, static_cast<unsigned long long>(peakFrameIndex));
        ImGui::Text("Captured Frame Index: %llu", static_cast<unsigned long long>(frameProfile.FrameIndex));
        ImGui::Separator();
        if (frameProfile.Events.empty())
        {
            ImGui::TextUnformatted("No CPU scopes captured yet.");
        }
        else
        {
            for (const Pragma::Core::ProfileEvent& event : frameProfile.Events)
            {
                ImGui::Indent(static_cast<float>(event.Depth) * 12.0f);
                ImGui::Text("%s: %.3f ms", event.Name.c_str(), event.DurationMilliseconds);
                ImGui::Unindent(static_cast<float>(event.Depth) * 12.0f);
            }
        }
        ImGui::Separator();
        ImGui::TextUnformatted("GPU");
        ImGui::Text("Last GPU Frame: %.3f ms", lastGpuFrameMilliseconds);
        ImGui::Text("Peak GPU Frame: %.3f ms (frame %llu)", peakGpuFrameMilliseconds, static_cast<unsigned long long>(peakGpuFrameIndex));
        ImGui::Text("Captured GPU Frame Index: %llu", static_cast<unsigned long long>(gpuFrameProfile.FrameIndex));
        ImGui::Separator();
        if (!gpuFrameProfile.IsValid || gpuFrameProfile.Events.empty())
        {
            ImGui::TextUnformatted("No GPU timings captured yet.");
        }
        else
        {
            for (const Pragma::Core::GpuProfileEvent& event : gpuFrameProfile.Events)
            {
                ImGui::Text("%s: %.3f ms", event.Name.c_str(), event.DurationMilliseconds);
            }
        }
        ImGui::End();
    }

    if (m_showLogConsoleWindow)
    {
        ImGui::Begin("Log Console", &m_showLogConsoleWindow);
        ImGui::Checkbox("Auto-scroll", &m_autoScrollLog);
        ImGui::Separator();
        ImGui::BeginChild("LogEntries");
        for (const Pragma::Core::LogEntry& entry : logEntries)
        {
            ImVec4 color = ImVec4(0.85f, 0.85f, 0.85f, 1.0f);
            if (entry.Level == Pragma::Core::LogLevel::Warning)
            {
                color = ImVec4(1.0f, 0.8f, 0.35f, 1.0f);
            }
            else if (entry.Level == Pragma::Core::LogLevel::Error)
            {
                color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
            }

            ImGui::TextColored(
                color,
                "[%s] [%s] [%s] %s",
                entry.Timestamp.c_str(),
                Pragma::Core::ToString(entry.Level),
                Pragma::Core::ToString(entry.Category),
                entry.Message.c_str());
        }
        if (m_autoScrollLog && ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 4.0f)
        {
            ImGui::SetScrollHereY(1.0f);
        }
        ImGui::EndChild();
        ImGui::End();
    }
}

void DebugUI::Render()
{
    if (!m_initialized)
    {
        return;
    }

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void DebugUI::ApplyPendingSceneActions(Pragma::Core::DemoScene& demoScene)
{
    if (!m_initialized)
    {
        return;
    }

    m_editorUi->ApplyPendingSceneActions(demoScene);
}

bool DebugUI::IsPhysicsOverlayEnabled() const noexcept
{
    return m_editorUi != nullptr && m_editorUi->IsPhysicsOverlayEnabled();
}
}
