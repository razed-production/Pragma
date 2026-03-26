#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include "Pragma/RHI/DX11/DX11Swapchain.h"

#include "Pragma/Core/Log.h"
#include "Pragma/RHI/DX11/DX11Device.h"

#include <stdexcept>

#include <windows.h>

namespace
{
DXGI_FORMAT ToDxgiFormat(const Pragma::RHI::PixelFormat format)
{
    switch (format)
    {
    case Pragma::RHI::PixelFormat::R8G8B8A8_UNorm:
        return DXGI_FORMAT_R8G8B8A8_UNORM;
    case Pragma::RHI::PixelFormat::D24_UNorm_S8_UInt:
        return DXGI_FORMAT_D24_UNORM_S8_UINT;
    case Pragma::RHI::PixelFormat::D32_Float:
        return DXGI_FORMAT_D32_FLOAT;
    default:
        return DXGI_FORMAT_UNKNOWN;
    }
}
}

namespace Pragma::RHI::DX11
{
DX11Swapchain::DX11Swapchain(DX11Device& device, SwapchainDesc desc)
    : m_device(device)
    , m_desc(desc)
{
    if (m_desc.Window.Handle == nullptr)
    {
        throw std::runtime_error("Swapchain creation requires a native window handle.");
    }

    DXGI_SWAP_CHAIN_DESC1 swapchainDesc{};
    swapchainDesc.Width = m_desc.Extent.Width;
    swapchainDesc.Height = m_desc.Extent.Height;
    swapchainDesc.Format = ToDxgiFormat(m_desc.ColorFormat);
    swapchainDesc.SampleDesc.Count = 1;
    swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchainDesc.BufferCount = m_desc.BufferCount;
    swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    swapchainDesc.Scaling = DXGI_SCALING_STRETCH;

    HRESULT result = m_device.GetFactory()->CreateSwapChainForHwnd(
        m_device.GetNativeDevice(),
        static_cast<HWND>(m_desc.Window.Handle),
        &swapchainDesc,
        nullptr,
        nullptr,
        &m_swapchain);

    if (FAILED(result))
    {
        throw std::runtime_error("Failed to create DX11 swapchain.");
    }

    m_device.GetFactory()->MakeWindowAssociation(static_cast<HWND>(m_desc.Window.Handle), DXGI_MWA_NO_ALT_ENTER);

    CreateViews();
}

DX11Swapchain::~DX11Swapchain()
{
    if (m_device.GetActiveSwapchain() == this)
    {
        m_device.SetActiveSwapchain(nullptr);
    }
}

const SwapchainDesc& DX11Swapchain::GetDesc() const noexcept
{
    return m_desc;
}

void DX11Swapchain::Resize(const Extent2D& newExtent)
{
    if (newExtent.Width == 0 || newExtent.Height == 0)
    {
        return;
    }

    Pragma::Core::Log(
        Pragma::Core::LogCategory::RHI,
        Pragma::Core::LogLevel::Info,
        "DX11Swapchain::Resize -> " + std::to_string(newExtent.Width) + "x" + std::to_string(newExtent.Height));

    ID3D11DeviceContext* context = m_device.GetImmediateContext();
    context->ClearState();
    context->Flush();
    Pragma::Core::Log(Pragma::Core::LogCategory::RHI, Pragma::Core::LogLevel::Info, "DX11Swapchain::Resize cleared device context state.");

    ReleaseViews();
    Pragma::Core::Log(Pragma::Core::LogCategory::RHI, Pragma::Core::LogLevel::Info, "DX11Swapchain::Resize released old views.");
    m_desc.Extent = newExtent;

    Pragma::Core::Log(Pragma::Core::LogCategory::RHI, Pragma::Core::LogLevel::Info, "DX11Swapchain::Resize calling ResizeBuffers.");
    HRESULT result = m_swapchain->ResizeBuffers(
        m_desc.BufferCount,
        m_desc.Extent.Width,
        m_desc.Extent.Height,
        ToDxgiFormat(m_desc.ColorFormat),
        0);

    if (FAILED(result))
    {
        throw std::runtime_error("Failed to resize DX11 swapchain buffers.");
    }

    Pragma::Core::Log(Pragma::Core::LogCategory::RHI, Pragma::Core::LogLevel::Info, "DX11Swapchain::Resize calling CreateViews.");
    CreateViews();
    Pragma::Core::Log(Pragma::Core::LogCategory::RHI, Pragma::Core::LogLevel::Info, "DX11Swapchain::Resize completed.");
}

void DX11Swapchain::Present()
{
    const UINT syncInterval = m_desc.VSyncEnabled ? 1u : 0u;
    m_swapchain->Present(syncInterval, 0);
}

void DX11Swapchain::BindRenderTargets()
{
    ID3D11RenderTargetView* renderTargetView = m_renderTargetView.Get();
    ID3D11DepthStencilView* depthStencilView = m_depthStencilView.Get();
    m_device.GetImmediateContext()->OMSetRenderTargets(1, &renderTargetView, depthStencilView);

    D3D11_VIEWPORT viewport{};
    viewport.Width = static_cast<float>(m_desc.Extent.Width);
    viewport.Height = static_cast<float>(m_desc.Extent.Height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    m_device.GetImmediateContext()->RSSetViewports(1, &viewport);
}

void DX11Swapchain::ClearColor(const ClearColorValue& color)
{
    const float values[4] = { color.R, color.G, color.B, color.A };
    m_device.GetImmediateContext()->ClearRenderTargetView(m_renderTargetView.Get(), values);
}

void DX11Swapchain::ClearDepth(const float depthValue)
{
    m_device.GetImmediateContext()->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH, depthValue, 0);
}

void DX11Swapchain::CreateViews()
{
    HRESULT result = m_swapchain->GetBuffer(0, IID_PPV_ARGS(&m_backBufferTexture));
    if (FAILED(result))
    {
        throw std::runtime_error("Failed to fetch DX11 swapchain back buffer.");
    }

    result = m_device.GetNativeDevice()->CreateRenderTargetView(m_backBufferTexture.Get(), nullptr, &m_renderTargetView);
    if (FAILED(result))
    {
        throw std::runtime_error("Failed to create DX11 render target view.");
    }

    D3D11_TEXTURE2D_DESC depthDesc{};
    depthDesc.Width = m_desc.Extent.Width;
    depthDesc.Height = m_desc.Extent.Height;
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = ToDxgiFormat(m_desc.DepthFormat);
    depthDesc.SampleDesc.Count = 1;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    result = m_device.GetNativeDevice()->CreateTexture2D(&depthDesc, nullptr, &m_depthTexture);
    if (FAILED(result))
    {
        throw std::runtime_error("Failed to create DX11 depth texture.");
    }

    result = m_device.GetNativeDevice()->CreateDepthStencilView(m_depthTexture.Get(), nullptr, &m_depthStencilView);
    if (FAILED(result))
    {
        throw std::runtime_error("Failed to create DX11 depth stencil view.");
    }
}

void DX11Swapchain::ReleaseViews()
{
    m_depthStencilView.Reset();
    m_depthTexture.Reset();
    m_renderTargetView.Reset();
    m_backBufferTexture.Reset();
}

ID3D11Texture2D* DX11Swapchain::GetBackBufferTexture() const noexcept
{
    return m_backBufferTexture.Get();
}
}
