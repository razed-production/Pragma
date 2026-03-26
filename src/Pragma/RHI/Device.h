#pragma once

#include "Pragma/RHI/BackendType.h"
#include "Pragma/RHI/Resources.h"
#include "Pragma/RHI/Types.h"

#include <cstdint>
#include <memory>
#include <string_view>

namespace Pragma::RHI
{
class ICommandList;
class ISwapchain;

class IDevice
{
public:
    virtual ~IDevice() = default;

    [[nodiscard]] virtual BackendType GetBackendType() const noexcept = 0;
    [[nodiscard]] virtual std::unique_ptr<ISwapchain> CreateSwapchain(const SwapchainDesc& desc) = 0;
    [[nodiscard]] virtual std::unique_ptr<ICommandList> CreateCommandList() = 0;
    [[nodiscard]] virtual std::unique_ptr<IBuffer> CreateBuffer(const BufferDesc& desc, const void* initialData) = 0;
    [[nodiscard]] virtual std::unique_ptr<ITexture> CreateTexture(const TextureDesc& desc, const TextureSubresourceData* initialData = nullptr) = 0;
    [[nodiscard]] virtual std::unique_ptr<IShader> CreateShader(const ShaderDesc& desc) = 0;
    [[nodiscard]] virtual std::unique_ptr<IPipelineState> CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) = 0;
    virtual void UpdateBuffer(IBuffer& buffer, const void* data, std::uint64_t sizeInBytes) = 0;
    virtual void Submit(ICommandList& commandList) = 0;
    virtual void BeginGpuFrameProfile(std::uint64_t) {}
    virtual void EndGpuFrameProfile() {}
    virtual void BeginGpuScope(std::string_view) {}
    virtual void EndGpuScope() {}
};
}
