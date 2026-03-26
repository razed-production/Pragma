#include "Pragma/RHI/Factory.h"

#include "Pragma/RHI/DX11/DX11Device.h"

#include <stdexcept>

namespace Pragma::RHI
{
std::unique_ptr<IDevice> CreateDevice(const BackendType backend)
{
    switch (backend)
    {
    case BackendType::Direct3D11:
        return std::make_unique<DX11::DX11Device>();

    case BackendType::Direct3D12:
    case BackendType::Vulkan:
        throw std::runtime_error("Requested backend is not implemented yet.");

    default:
        throw std::runtime_error("Unknown backend requested.");
    }
}
}
