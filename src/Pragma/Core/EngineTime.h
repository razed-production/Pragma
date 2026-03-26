#pragma once

#include <cstdint>

namespace Pragma::Core
{
struct EngineTime
{
    float DeltaSeconds = 0.0f;
    float ElapsedSeconds = 0.0f;
    std::uint64_t FrameIndex = 0;
};
}
