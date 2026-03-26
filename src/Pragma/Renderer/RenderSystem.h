#pragma once

#include "Pragma/Core/EngineConfig.h"
#include "Pragma/Platform/InputState.h"
#include "Pragma/Renderer/Scene.h"

#include "Pragma/RHI/BackendType.h"
#include "Pragma/RHI/Types.h"

#include <cstdint>
#include <functional>
#include <memory>

namespace Pragma::RHI
{
class ICommandList;
class ISwapchain;
class IBuffer;
class ITexture;
class IPipelineState;
}

namespace Pragma::RHI
{
class IDevice;
}

namespace Pragma::Renderer
{
struct Mesh;
struct Material;

struct RenderStatistics
{
    std::uint64_t SceneObjectCount = 0;
    std::uint64_t MeshRendererCount = 0;
    std::uint64_t CachedWorldTransformCount = 0;
    std::uint64_t RenderProxyCount = 0;
    std::uint64_t MainPassVisibleMeshCount = 0;
    std::uint64_t MainPassCulledMeshCount = 0;
    std::uint64_t ShadowPassVisibleMeshCount = 0;
    std::uint64_t ShadowPassCulledMeshCount = 0;
    std::uint64_t ShadowPassLodSkippedMeshCount = 0;
    std::uint64_t PhysicsOverlayObjectCount = 0;
    std::uint64_t LodOverlayObjectCount = 0;
    std::uint64_t HighLodMeshCount = 0;
    std::uint64_t MediumLodMeshCount = 0;
    std::uint64_t LowLodMeshCount = 0;
    std::uint64_t MainPassDrawCalls = 0;
    std::uint64_t ShadowPassDrawCalls = 0;
    std::uint64_t SkyDrawCalls = 0;
    std::uint64_t BloomDrawCalls = 0;
    std::uint64_t PhysicsOverlayDrawCalls = 0;
    std::uint64_t LodOverlayDrawCalls = 0;
    std::uint64_t TonemapDrawCalls = 0;
    std::uint64_t TotalDrawCalls = 0;
    std::uint64_t InstancedDrawCalls = 0;
    std::uint64_t InstancedInstanceCount = 0;
    std::uint64_t MainPassTriangles = 0;
    std::uint64_t ShadowPassTriangles = 0;
    std::uint64_t SkyTriangles = 0;
    std::uint64_t BloomTriangles = 0;
    std::uint64_t PhysicsOverlayTriangles = 0;
    std::uint64_t TonemapTriangles = 0;
    std::uint64_t TotalTriangles = 0;
    std::uint64_t PipelineBinds = 0;
    std::uint64_t VertexBufferBinds = 0;
    std::uint64_t IndexBufferBinds = 0;
    std::uint64_t MaterialBufferBinds = 0;
    std::uint64_t TextureBinds = 0;
    std::uint32_t InternalRenderWidth = 0;
    std::uint32_t InternalRenderHeight = 0;
    std::uint32_t ShadowMapResolution = 0;
    Pragma::Core::GraphicsQualityPreset QualityPreset = Pragma::Core::GraphicsQualityPreset::Balanced;
    Pragma::Core::ShadingQualityTier ShadingQuality = Pragma::Core::ShadingQualityTier::Balanced;
    float ShadowDistance = 0.0f;
    float ShadowHalfExtent = 0.0f;
    float RenderScale = 1.0f;
    float BloomResolutionScale = 0.25f;
    std::uint32_t BloomResolutionWidth = 0;
    std::uint32_t BloomResolutionHeight = 0;
    bool FxaaEnabled = false;
    bool LodEnabled = false;
};

class RenderSystem
{
public:
    RenderSystem(
        Pragma::RHI::IDevice& device,
        Pragma::RHI::NativeWindow window,
        Pragma::RHI::Extent2D extent,
        const Pragma::Core::GraphicsConfig& graphicsConfig);
    ~RenderSystem();

    void Initialize();
    void Resize(Pragma::RHI::Extent2D extent);
    void RenderFrame(const Scene& scene, bool showPhysicsOverlay, bool showLodOverlay, const std::function<void()>& overlayCallback = {});
    [[nodiscard]] Pragma::RHI::BackendType GetBackendType() const noexcept;
    [[nodiscard]] const RenderStatistics& GetLastFrameStatistics() const noexcept;
    [[nodiscard]] Pragma::Core::GraphicsQualityPreset GetGraphicsQualityPreset() const noexcept;
    [[nodiscard]] const Pragma::Core::GraphicsConfig& GetGraphicsConfig() const noexcept;
    void SetGraphicsQualityPreset(Pragma::Core::GraphicsQualityPreset preset);
    void ApplyGraphicsConfig(const Pragma::Core::GraphicsConfig& graphicsConfig);

private:
    void DrawSceneToCurrentTargets(const Scene& scene, Pragma::RHI::Extent2D extent, bool showPhysicsOverlay, bool showLodOverlay);
    void CreateRenderTargets(Pragma::RHI::Extent2D extent);

private:
    Pragma::RHI::IDevice& m_device;
    std::unique_ptr<Pragma::RHI::ISwapchain> m_swapchain;
    std::unique_ptr<Pragma::RHI::ICommandList> m_commandList;
    std::unique_ptr<Pragma::RHI::IBuffer> m_frameConstantBuffer;
    std::unique_ptr<Pragma::RHI::IBuffer> m_shadowConstantBuffer;
    std::unique_ptr<Pragma::RHI::IBuffer> m_skyConstantBuffer;
    std::unique_ptr<Pragma::RHI::IBuffer> m_bloomConstantBuffer;
    std::unique_ptr<Pragma::RHI::IBuffer> m_tonemapConstantBuffer;
    std::unique_ptr<Pragma::RHI::IBuffer> m_mainInstanceConstantBuffer;
    std::unique_ptr<Pragma::RHI::IPipelineState> m_shadowPipeline;
    std::unique_ptr<Pragma::RHI::IPipelineState> m_skyPipeline;
    std::unique_ptr<Pragma::RHI::IPipelineState> m_bloomPipeline;
    std::unique_ptr<Pragma::RHI::IPipelineState> m_tonemapPipeline;
    std::unique_ptr<Pragma::RHI::ITexture> m_hdrColorTarget;
    std::unique_ptr<Pragma::RHI::ITexture> m_hdrDepthTarget;
    std::unique_ptr<Pragma::RHI::ITexture> m_bloomColorTarget;
    std::unique_ptr<Pragma::RHI::ITexture> m_shadowColorTarget;
    std::unique_ptr<Pragma::RHI::ITexture> m_shadowDepthTarget;
    Pragma::RHI::SwapchainDesc m_swapchainDesc;
    std::shared_ptr<Mesh> m_debugCubeMesh;
    std::shared_ptr<Material> m_physicsDebugMaterial;
    std::shared_ptr<Material> m_lodDebugMaterial;
    RenderStatistics m_lastFrameStatistics;
    Pragma::Core::GraphicsConfig m_graphicsConfig;
};
}
