#pragma once

#include <cstdint>
#include <vector>

namespace Pragma::Assets
{
struct ImageAssetData
{
    std::uint32_t Width = 0;
    std::uint32_t Height = 0;
    std::vector<std::uint8_t> Pixels;
};
}
