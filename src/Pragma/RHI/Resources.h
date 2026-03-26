#pragma once

#include "Pragma/RHI/Types.h"

namespace Pragma::RHI
{
class IDeviceChild
{
public:
    virtual ~IDeviceChild() = default;

    [[nodiscard]] virtual const char* GetDebugName() const noexcept = 0;
};

class IBuffer : public IDeviceChild
{
public:
    ~IBuffer() override = default;

    [[nodiscard]] virtual const BufferDesc& GetDesc() const noexcept = 0;
};

class ITexture : public IDeviceChild
{
public:
    ~ITexture() override = default;

    [[nodiscard]] virtual const TextureDesc& GetDesc() const noexcept = 0;
};

class IShader : public IDeviceChild
{
public:
    ~IShader() override = default;

    [[nodiscard]] virtual ShaderStage GetStage() const noexcept = 0;
    [[nodiscard]] virtual const ShaderDesc& GetDesc() const noexcept = 0;
};

class IPipelineState : public IDeviceChild
{
public:
    ~IPipelineState() override = default;

    [[nodiscard]] virtual const GraphicsPipelineDesc& GetDesc() const noexcept = 0;
};
}
