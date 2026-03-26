#pragma once

#include "Pragma/RHI/Device.h"

#include <d3d11.h>
#include <dxgi1_2.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

#include <array>
#include <cstdint>
#include <string>

namespace Pragma::RHI::DX11
{
class DX11Swapchain;

class DX11Device final : public IDevice
{
public:
    DX11Device();

    [[nodiscard]] BackendType GetBackendType() const noexcept override;
    [[nodiscard]] std::unique_ptr<ISwapchain> CreateSwapchain(const SwapchainDesc& desc) override;
    [[nodiscard]] std::unique_ptr<ICommandList> CreateCommandList() override;
    [[nodiscard]] std::unique_ptr<IBuffer> CreateBuffer(const BufferDesc& desc, const void* initialData) override;
    [[nodiscard]] std::unique_ptr<ITexture> CreateTexture(const TextureDesc& desc, const TextureSubresourceData* initialData = nullptr) override;
    [[nodiscard]] std::unique_ptr<IShader> CreateShader(const ShaderDesc& desc) override;
    [[nodiscard]] std::unique_ptr<IPipelineState> CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) override;
    void UpdateBuffer(IBuffer& buffer, const void* data, std::uint64_t sizeInBytes) override;
    void Submit(ICommandList& commandList) override;

    [[nodiscard]] ID3D11Device* GetNativeDevice() const noexcept;
    [[nodiscard]] ID3D11DeviceContext* GetImmediateContext() const noexcept;
    [[nodiscard]] ID3D11SamplerState* GetDefaultSampler() const noexcept;
    [[nodiscard]] ID3D11SamplerState* GetShadowSampler() const noexcept;
    [[nodiscard]] IDXGIFactory2* GetFactory() const noexcept;
    [[nodiscard]] DX11Swapchain* GetActiveSwapchain() const noexcept;
    void SetActiveSwapchain(DX11Swapchain* swapchain) noexcept;
    void BeginGpuFrameProfile(std::uint64_t frameIndex) override;
    void EndGpuFrameProfile() override;
    void BeginGpuScope(std::string_view name) override;
    void EndGpuScope() override;

private:
    [[nodiscard]] Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(const ShaderDesc& desc) const;
    void DrainDebugMessages();
    void ResolveGpuFrameProfiles();

    struct GpuFrameQueries
    {
        struct GpuScopeQueries
        {
            Microsoft::WRL::ComPtr<ID3D11Query> StartTimestampQuery;
            Microsoft::WRL::ComPtr<ID3D11Query> EndTimestampQuery;
            std::string Name;
            bool Used = false;
        };

        Microsoft::WRL::ComPtr<ID3D11Query> DisjointQuery;
        Microsoft::WRL::ComPtr<ID3D11Query> StartTimestampQuery;
        Microsoft::WRL::ComPtr<ID3D11Query> EndTimestampQuery;
        std::array<GpuScopeQueries, 16> ScopeQueries{};
        std::uint64_t FrameIndex = 0;
        std::uint32_t ScopeCount = 0;
        std::uint32_t ResolveCooldownFrames = 0;
        std::int32_t ActiveScopeIndex = -1;
        bool CaptureOpen = false;
        bool ResolvePending = false;
    };

private:
    static constexpr std::size_t kGpuProfileLatency = 4;

    Microsoft::WRL::ComPtr<ID3D11Device> m_device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_context;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> m_defaultSampler;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> m_shadowSampler;
    Microsoft::WRL::ComPtr<ID3D11InfoQueue> m_infoQueue;
    Microsoft::WRL::ComPtr<IDXGIFactory2> m_factory;
    DX11Swapchain* m_activeSwapchain = nullptr;
    unsigned long long m_lastDebugMessageCount = 0;
    std::array<GpuFrameQueries, kGpuProfileLatency> m_gpuFrameQueries{};
    std::size_t m_gpuFrameWriteIndex = 0;
    std::int32_t m_activeGpuFrameQueryIndex = -1;
    bool m_gpuProfilingEnabled = false;
    bool m_gpuBaselineCaptured = false;
    bool m_gpuProfilingSaturatedWarningLogged = false;
};
}
