#pragma once

#include "Pragma/RHI/Resources.h"
#include "Pragma/RHI/Types.h"

namespace Pragma::RHI
{
class ICommandList
{
public:
    virtual ~ICommandList() = default;

    virtual void Begin() = 0;
    virtual void End() = 0;
    virtual void SetRenderTargets(ITexture* colorTarget, ITexture* depthTarget) = 0;
    virtual void ClearColor(const ClearColorValue& color) = 0;
    virtual void ClearDepth(float depthValue) = 0;
    virtual void SetGraphicsPipeline(IPipelineState& pipeline) = 0;
    virtual void SetVertexBuffer(IBuffer& buffer, std::uint64_t offset) = 0;
    virtual void SetIndexBuffer(IBuffer& buffer, IndexFormat format, std::uint64_t offset) = 0;
    virtual void SetConstantBuffer(std::uint32_t slot, IBuffer& buffer) = 0;
    virtual void SetTexture(std::uint32_t slot, ITexture* texture) = 0;
    virtual void Draw(std::uint32_t vertexCount, std::uint32_t firstVertex) = 0;
    virtual void DrawIndexed(std::uint32_t indexCount, std::uint32_t firstIndex, std::int32_t vertexOffset) = 0;
};
}
