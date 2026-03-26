#include "Pragma/Assets/AssetManager.h"
#include "Pragma/Assets/AssetManifest.h"
#include "Pragma/Core/DemoScene.h"
#include "Pragma/Core/EngineInput.h"
#include "Pragma/Core/EngineTime.h"
#include "Pragma/Core/Log.h"
#include "Pragma/Core/Profiler.h"
#include "Pragma/Core/Application.h"
#include "Pragma/DebugUI/DebugUI.h"
#include "Pragma/Physics/PhysicsSystem.h"
#include "Pragma/Scripting/ManagedScriptHost.h"

#include "Pragma/Platform/Window.h"
#include "Pragma/RHI/Device.h"
#include "Pragma/RHI/Factory.h"
#include "Pragma/Renderer/CameraSystem.h"
#include "Pragma/Renderer/RenderSystem.h"

#include <chrono>
#include <filesystem>
#include <iterator>
#include <memory>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace
{
std::filesystem::path GetExecutableDirectory()
{
    wchar_t buffer[MAX_PATH]{};
    const DWORD length = GetModuleFileNameW(nullptr, buffer, static_cast<DWORD>(std::size(buffer)));
    return std::filesystem::path(std::wstring(buffer, length)).parent_path();
}
}

namespace Pragma::Core
{
Application::Application() = default;

Application::~Application() = default;

void Application::Run()
{
    Log(LogCategory::General, LogLevel::Info, "Pragma bootstrap starting.");
    Log(LogCategory::General, LogLevel::Info, "Target backend: " + std::string(RHI::ToString(m_config.Graphics.Backend)));

    Platform::Window window(L"Pragma", 1600, 900);
    std::unique_ptr<RHI::IDevice> device = RHI::CreateDevice(m_config.Graphics.Backend);
    Renderer::RenderSystem renderer(*device, window.GetNativeWindow(), window.GetExtent(), m_config.Graphics);
    Renderer::CameraSystem cameraSystem;
    Physics::PhysicsSystem physicsSystem;
    Scripting::ManagedScriptHost managedScriptHost;
    Pragma::DebugUI::DebugUI debugUi;
    const std::filesystem::path assetRoot = GetExecutableDirectory().parent_path().parent_path();
    const Assets::AssetManifest manifest = Assets::AssetManifest::LoadFromFile(assetRoot / "assets" / "manifest.txt");
    Assets::AssetManager assets(*device, manifest);
    managedScriptHost.Initialize(assets);
    DemoScene demoScene(*device, assets, managedScriptHost);
    demoScene.Initialize();

    renderer.Initialize();
    physicsSystem.Initialize();
    debugUi.Initialize(*device, window.GetNativeWindow());
    window.SetMessageHandler([&debugUi](void* hwnd, unsigned int message, std::size_t wParam, std::ptrdiff_t lParam)
    {
        return debugUi.HandleWindowMessage(hwnd, message, wParam, lParam);
    });
    auto previousTick = std::chrono::steady_clock::now();
    float elapsedSeconds = 0.0f;
    std::uint64_t frameIndex = 0;
    Pragma::RHI::Extent2D appliedExtent = window.GetExtent();
    Pragma::RHI::Extent2D pendingExtent = appliedExtent;
    float pendingResizeCooldownSeconds = 0.0f;

    while (window.PumpMessages())
    {
        const auto frameStart = std::chrono::steady_clock::now();
        const auto currentTick = std::chrono::steady_clock::now();
        const float deltaSeconds = std::chrono::duration<float>(currentTick - previousTick).count();
        previousTick = currentTick;
        elapsedSeconds += deltaSeconds;
        ++frameIndex;
        BeginProfileFrame(frameIndex);

        const EngineTime time
        {
            deltaSeconds,
            elapsedSeconds,
            frameIndex
        };
        const EngineInput input(window.GetInputState());
        PRAGMA_PROFILE_SCOPE("Frame");

        {
            PRAGMA_PROFILE_SCOPE("Resize");
            const Pragma::RHI::Extent2D currentExtent = window.GetExtent();
            if (currentExtent.Width > 0 && currentExtent.Height > 0)
            {
                if (currentExtent.Width != pendingExtent.Width || currentExtent.Height != pendingExtent.Height)
                {
                    pendingExtent = currentExtent;
                    pendingResizeCooldownSeconds = 0.25f;
                }

                pendingResizeCooldownSeconds = std::max(0.0f, pendingResizeCooldownSeconds - deltaSeconds);
                if ((pendingExtent.Width != appliedExtent.Width || pendingExtent.Height != appliedExtent.Height) &&
                    pendingResizeCooldownSeconds <= 0.0f)
                {
                    renderer.Resize(pendingExtent);
                    appliedExtent = pendingExtent;
                    Pragma::Core::Log(
                        Pragma::Core::LogCategory::Renderer,
                        Pragma::Core::LogLevel::Info,
                        "Applied stabilized resize to " +
                        std::to_string(appliedExtent.Width) + "x" +
                        std::to_string(appliedExtent.Height) +
                        (window.IsFullscreen() ? " (fullscreen)" : " (windowed)"));
                }
            }
        }
        {
            PRAGMA_PROFILE_SCOPE("Scene Update");
            demoScene.Update(time, input);
        }
        {
            PRAGMA_PROFILE_SCOPE("PhysicsSystem");
            physicsSystem.Update(demoScene.GetScene(), time);
        }
        {
            PRAGMA_PROFILE_SCOPE("CameraSystem");
            cameraSystem.Update(demoScene.GetScene(), time, input);
        }
        {
            PRAGMA_PROFILE_SCOPE("DebugUI BeginFrame");
            debugUi.BeginFrame(deltaSeconds, demoScene, renderer, physicsSystem, managedScriptHost);
        }
        {
            PRAGMA_PROFILE_SCOPE("Render");
            renderer.RenderFrame(demoScene.GetScene(), debugUi.IsPhysicsOverlayEnabled(), debugUi.IsLodOverlayEnabled(), [&debugUi]()
            {
                debugUi.Render();
            });
        }
        {
            PRAGMA_PROFILE_SCOPE("Window EndFrame");
            window.EndFrame();
        }
        debugUi.ApplyPendingSceneActions(demoScene);

        const auto frameEnd = std::chrono::steady_clock::now();
        const double frameMilliseconds = std::chrono::duration<double, std::milli>(frameEnd - frameStart).count();
        EndProfileFrame(frameMilliseconds);
    }

    physicsSystem.Shutdown();
    managedScriptHost.Shutdown();
    Log(LogCategory::General, LogLevel::Info, "Pragma bootstrap finished.");
}
}
