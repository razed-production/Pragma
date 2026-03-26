#pragma once

#include "Pragma/RHI/Types.h"

namespace Pragma::RHI
{
class ICommandList;

class ISwapchain
{
public:
    virtual ~ISwapchain() = default;

    [[nodiscard]] virtual const SwapchainDesc& GetDesc() const noexcept = 0;
    virtual void Resize(const Extent2D& newExtent) = 0;
    virtual void Present() = 0;
};
}
