#pragma once

#include "Pragma/RHI/CommandList.h"

#include <d3d11.h>

namespace Pragma::RHI::DX11
{
class DX11Device;

class DX11CommandList final : public ICommandList
{
public:
    explicit DX11CommandList(DX11Device& device);

    void Begin() override;
    void End() override;
    void SetRenderTargets(ITexture* colorTarget, ITexture* depthTarget) override;
    void ClearColor(const ClearColorValue& color) override;
    void ClearDepth(float depthValue) override;
    void SetGraphicsPipeline(IPipelineState& pipeline) override;
    void SetVertexBuffer(IBuffer& buffer, std::uint64_t offset) override;
    void SetIndexBuffer(IBuffer& buffer, IndexFormat format, std::uint64_t offset) override;
    void SetConstantBuffer(std::uint32_t slot, IBuffer& buffer) override;
    void SetTexture(std::uint32_t slot, ITexture* texture) override;
    void Draw(std::uint32_t vertexCount, std::uint32_t firstVertex) override;
    void DrawIndexed(std::uint32_t indexCount, std::uint32_t firstIndex, std::int32_t vertexOffset) override;
    void DrawIndexedInstanced(std::uint32_t indexCountPerInstance, std::uint32_t instanceCount, std::uint32_t firstIndex, std::int32_t vertexOffset, std::uint32_t firstInstance) override;

private:
    DX11Device& m_device;
    bool m_hasCustomRenderTargets = false;
    ID3D11RenderTargetView* m_customRenderTargetView = nullptr;
    ID3D11DepthStencilView* m_customDepthStencilView = nullptr;
};
}
