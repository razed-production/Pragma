#pragma once

#include "Pragma/Platform/InputState.h"
#include "Pragma/Renderer/Scene.h"

#include "Pragma/RHI/BackendType.h"
#include "Pragma/RHI/Types.h"

#include <functional>
#include <memory>

namespace Pragma::RHI
{
class ICommandList;
class ISwapchain;
class IBuffer;
}

namespace Pragma::RHI
{
class IDevice;
}

namespace Pragma::Renderer
{
struct Mesh;
struct Material;

class RenderSystem
{
public:
    RenderSystem(Pragma::RHI::IDevice& device, Pragma::RHI::NativeWindow window, Pragma::RHI::Extent2D extent);
    ~RenderSystem();

    void Initialize();
    void Resize(Pragma::RHI::Extent2D extent);
    void RenderFrame(const Scene& scene, bool showPhysicsOverlay, const std::function<void()>& overlayCallback = {});
    [[nodiscard]] Pragma::RHI::BackendType GetBackendType() const noexcept;

private:
    void DrawSceneToCurrentTargets(const Scene& scene, Pragma::RHI::Extent2D extent, bool showPhysicsOverlay);

private:
    Pragma::RHI::IDevice& m_device;
    std::unique_ptr<Pragma::RHI::ISwapchain> m_swapchain;
    std::unique_ptr<Pragma::RHI::ICommandList> m_commandList;
    std::unique_ptr<Pragma::RHI::IBuffer> m_frameConstantBuffer;
    Pragma::RHI::SwapchainDesc m_swapchainDesc;
    std::shared_ptr<Mesh> m_debugCubeMesh;
    std::shared_ptr<Material> m_physicsDebugMaterial;
};
}
