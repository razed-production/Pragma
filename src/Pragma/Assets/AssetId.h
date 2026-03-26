#pragma once

#include <string>

namespace Pragma::Assets
{
struct AssetId
{
    std::string Value;

    [[nodiscard]] bool empty() const noexcept
    {
        return Value.empty();
    }
};
}
