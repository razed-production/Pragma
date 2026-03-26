#pragma once

#include <cstddef>
#include <string>
#include <memory>

namespace Pragma::RHI
{
class IDevice;
struct NativeWindow;
}

namespace Pragma::Core
{
class DemoScene;
}

namespace Pragma::Physics
{
class PhysicsSystem;
}

namespace Pragma::Scripting
{
class ManagedScriptHost;
}

namespace Pragma::Editor
{
class EditorUI;
}

namespace Pragma::DebugUI
{
class DebugUI
{
public:
    DebugUI();
    ~DebugUI();

    void Initialize(Pragma::RHI::IDevice& device, Pragma::RHI::NativeWindow window);
    void Shutdown();

    [[nodiscard]] bool HandleWindowMessage(void* hwnd, unsigned int message, std::size_t wParam, std::ptrdiff_t lParam);
    void BeginFrame(
        float deltaSeconds,
        Pragma::Core::DemoScene& demoScene,
        Pragma::Physics::PhysicsSystem& physicsSystem,
        Pragma::Scripting::ManagedScriptHost& managedScriptHost);
    void Render();
    void ApplyPendingSceneActions(Pragma::Core::DemoScene& demoScene);
    [[nodiscard]] bool IsPhysicsOverlayEnabled() const noexcept;

private:
    bool m_initialized = false;
    bool m_autoScrollLog = true;
    bool m_showDiagnosticsWindow = true;
    bool m_showProfilerWindow = true;
    bool m_showLogConsoleWindow = true;
    std::string m_imguiIniPath;
    std::unique_ptr<Pragma::Editor::EditorUI> m_editorUi;
};
}
