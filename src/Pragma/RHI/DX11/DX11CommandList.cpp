#include "Pragma/RHI/DX11/DX11CommandList.h"

#include "Pragma/Core/Profiler.h"
#include "Pragma/RHI/DX11/DX11Device.h"
#include "Pragma/RHI/DX11/DX11Resources.h"
#include "Pragma/RHI/DX11/DX11Swapchain.h"

#include <stdexcept>

namespace
{
D3D11_PRIMITIVE_TOPOLOGY ToDxTopology(const Pragma::RHI::PrimitiveTopology topology)
{
    switch (topology)
    {
    case Pragma::RHI::PrimitiveTopology::TriangleList:
        return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    case Pragma::RHI::PrimitiveTopology::TriangleStrip:
        return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
    case Pragma::RHI::PrimitiveTopology::LineList:
        return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
    default:
        return D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    }
}

DXGI_FORMAT ToDxgiIndexFormat(const Pragma::RHI::IndexFormat format)
{
    return format == Pragma::RHI::IndexFormat::UInt16 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
}
}

namespace Pragma::RHI::DX11
{
DX11CommandList::DX11CommandList(DX11Device& device)
    : m_device(device)
{
}

void DX11CommandList::Begin()
{
    m_device.BeginGpuFrameProfile(Pragma::Core::GetCurrentProfileFrameIndex());
}

void DX11CommandList::End()
{
    m_device.EndGpuFrameProfile();
}

void DX11CommandList::SetRenderTargets(ITexture* colorTarget, ITexture* depthTarget)
{
    if (colorTarget == nullptr && depthTarget == nullptr)
    {
        m_hasCustomRenderTargets = false;
        m_customRenderTargetView = nullptr;
        m_customDepthStencilView = nullptr;

        DX11Swapchain* swapchain = m_device.GetActiveSwapchain();
        if (swapchain == nullptr)
        {
            throw std::runtime_error("DX11 command list requires an active swapchain.");
        }

        swapchain->BindRenderTargets();
        return;
    }

    auto* dxColor = colorTarget != nullptr ? dynamic_cast<DX11Texture*>(colorTarget) : nullptr;
    auto* dxDepth = depthTarget != nullptr ? dynamic_cast<DX11Texture*>(depthTarget) : nullptr;
    if (dxColor == nullptr)
    {
        throw std::runtime_error("DX11 command list received invalid custom render targets.");
    }

    m_customRenderTargetView = dxColor->GetRenderTargetView();
    m_customDepthStencilView = dxDepth != nullptr ? dxDepth->GetDepthStencilView() : nullptr;
    m_hasCustomRenderTargets = true;

    if (m_customRenderTargetView == nullptr)
    {
        throw std::runtime_error("DX11 command list custom render targets are missing RTV.");
    }

    ID3D11DeviceContext* context = m_device.GetImmediateContext();
    context->OMSetRenderTargets(1, &m_customRenderTargetView, m_customDepthStencilView);

    D3D11_VIEWPORT viewport{};
    viewport.Width = static_cast<float>(dxColor->GetDesc().Width);
    viewport.Height = static_cast<float>(dxColor->GetDesc().Height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    context->RSSetViewports(1, &viewport);
}

void DX11CommandList::ClearColor(const ClearColorValue& color)
{
    if (!m_hasCustomRenderTargets)
    {
        m_device.GetActiveSwapchain()->ClearColor(color);
        return;
    }

    const float values[4] = { color.R, color.G, color.B, color.A };
    m_device.GetImmediateContext()->ClearRenderTargetView(m_customRenderTargetView, values);
}

void DX11CommandList::ClearDepth(const float depthValue)
{
    if (!m_hasCustomRenderTargets)
    {
        m_device.GetActiveSwapchain()->ClearDepth(depthValue);
        return;
    }

    if (m_customDepthStencilView == nullptr)
    {
        return;
    }

    m_device.GetImmediateContext()->ClearDepthStencilView(m_customDepthStencilView, D3D11_CLEAR_DEPTH, depthValue, 0);
}

void DX11CommandList::SetGraphicsPipeline(IPipelineState& pipeline)
{
    auto& dxPipeline = dynamic_cast<DX11PipelineState&>(pipeline);
    ID3D11DeviceContext* context = m_device.GetImmediateContext();

    context->IASetPrimitiveTopology(ToDxTopology(dxPipeline.GetDesc().Topology));
    context->IASetInputLayout(dxPipeline.GetInputLayout());
    context->VSSetShader(dxPipeline.GetVertexShader(), nullptr, 0);
    context->PSSetShader(dxPipeline.GetPixelShader(), nullptr, 0);
    context->RSSetState(dxPipeline.GetRasterizerState());
    context->OMSetDepthStencilState(dxPipeline.GetDepthStencilState(), 0);

    constexpr float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    context->OMSetBlendState(dxPipeline.GetBlendState(), blendFactor, 0xFFFFFFFFu);

    ID3D11SamplerState* samplers[2] =
    {
        m_device.GetDefaultSampler(),
        m_device.GetShadowSampler()
    };
    context->PSSetSamplers(0, 2, samplers);
}

void DX11CommandList::SetVertexBuffer(IBuffer& buffer, const std::uint64_t offset)
{
    auto& dxBuffer = dynamic_cast<DX11Buffer&>(buffer);
    ID3D11Buffer* nativeBuffer = dxBuffer.GetNativeBuffer();
    const UINT stride = dxBuffer.GetDesc().Stride;
    const UINT byteOffset = static_cast<UINT>(offset);

    m_device.GetImmediateContext()->IASetVertexBuffers(0, 1, &nativeBuffer, &stride, &byteOffset);
}

void DX11CommandList::SetIndexBuffer(IBuffer& buffer, const IndexFormat format, const std::uint64_t offset)
{
    auto& dxBuffer = dynamic_cast<DX11Buffer&>(buffer);
    m_device.GetImmediateContext()->IASetIndexBuffer(
        dxBuffer.GetNativeBuffer(),
        ToDxgiIndexFormat(format),
        static_cast<UINT>(offset));
}

void DX11CommandList::SetConstantBuffer(const std::uint32_t slot, IBuffer& buffer)
{
    auto& dxBuffer = dynamic_cast<DX11Buffer&>(buffer);
    ID3D11Buffer* nativeBuffer = dxBuffer.GetNativeBuffer();

    ID3D11DeviceContext* context = m_device.GetImmediateContext();
    context->VSSetConstantBuffers(slot, 1, &nativeBuffer);
    context->PSSetConstantBuffers(slot, 1, &nativeBuffer);
}

void DX11CommandList::SetTexture(const std::uint32_t slot, ITexture* texture)
{
    ID3D11ShaderResourceView* shaderResourceView = nullptr;

    if (texture != nullptr)
    {
        auto& dxTexture = dynamic_cast<DX11Texture&>(*texture);
        shaderResourceView = dxTexture.GetShaderResourceView();
    }

    m_device.GetImmediateContext()->PSSetShaderResources(slot, 1, &shaderResourceView);
}

void DX11CommandList::Draw(const std::uint32_t vertexCount, const std::uint32_t firstVertex)
{
    m_device.GetImmediateContext()->Draw(vertexCount, firstVertex);
}

void DX11CommandList::DrawIndexed(const std::uint32_t indexCount, const std::uint32_t firstIndex, const std::int32_t vertexOffset)
{
    m_device.GetImmediateContext()->DrawIndexed(indexCount, firstIndex, vertexOffset);
}

void DX11CommandList::DrawIndexedInstanced(
    const std::uint32_t indexCountPerInstance,
    const std::uint32_t instanceCount,
    const std::uint32_t firstIndex,
    const std::int32_t vertexOffset,
    const std::uint32_t firstInstance)
{
    m_device.GetImmediateContext()->DrawIndexedInstanced(indexCountPerInstance, instanceCount, firstIndex, vertexOffset, firstInstance);
}
}
