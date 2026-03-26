#pragma once

#include <cstdint>

namespace Pragma::Platform
{
struct InputState
{
    bool MoveForward = false;
    bool MoveBackward = false;
    bool MoveLeft = false;
    bool MoveRight = false;
    bool MoveUp = false;
    bool MoveDown = false;
    bool LookLeft = false;
    bool LookRight = false;
    bool LookUp = false;
    bool LookDown = false;
    bool FastMove = false;
    bool RightMouseButtonDown = false;
    std::int32_t MouseDeltaX = 0;
    std::int32_t MouseDeltaY = 0;
};
}
