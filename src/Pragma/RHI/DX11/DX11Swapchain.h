#pragma once

#include "Pragma/RHI/Swapchain.h"

#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

namespace Pragma::RHI::DX11
{
class DX11Device;

class DX11Swapchain final : public ISwapchain
{
public:
    DX11Swapchain(DX11Device& device, SwapchainDesc desc);
    ~DX11Swapchain() override;

    [[nodiscard]] const SwapchainDesc& GetDesc() const noexcept override;
    void Resize(const Extent2D& newExtent) override;
    void Present() override;

    void BindRenderTargets();
    void ClearColor(const ClearColorValue& color);
    void ClearDepth(float depthValue);
    [[nodiscard]] ID3D11Texture2D* GetBackBufferTexture() const noexcept;

private:
    void CreateViews();
    void ReleaseViews();

private:
    DX11Device& m_device;
    SwapchainDesc m_desc;
    Microsoft::WRL::ComPtr<IDXGISwapChain1> m_swapchain;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_backBufferTexture;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_renderTargetView;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_depthTexture;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_depthStencilView;
};
}
