#pragma once

namespace Pragma::RHI
{
enum class BackendType
{
    Direct3D11,
    Direct3D12,
    Vulkan
};

[[nodiscard]] constexpr const char* ToString(const BackendType backend) noexcept
{
    switch (backend)
    {
    case BackendType::Direct3D11:
        return "Direct3D11";
    case BackendType::Direct3D12:
        return "Direct3D12";
    case BackendType::Vulkan:
        return "Vulkan";
    default:
        return "Unknown";
    }
}
}
