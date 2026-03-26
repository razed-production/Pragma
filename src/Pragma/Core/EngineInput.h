#pragma once

#include "Pragma/Platform/InputState.h"

namespace Pragma::Core
{
class EngineInput
{
public:
    explicit EngineInput(const Pragma::Platform::InputState& state) noexcept
        : m_state(&state)
    {
    }

    [[nodiscard]] const Pragma::Platform::InputState& GetPlatformState() const noexcept
    {
        return *m_state;
    }

    [[nodiscard]] bool IsMoveForwardPressed() const noexcept { return m_state->MoveForward; }
    [[nodiscard]] bool IsMoveBackwardPressed() const noexcept { return m_state->MoveBackward; }
    [[nodiscard]] bool IsMoveLeftPressed() const noexcept { return m_state->MoveLeft; }
    [[nodiscard]] bool IsMoveRightPressed() const noexcept { return m_state->MoveRight; }
    [[nodiscard]] bool IsMoveUpPressed() const noexcept { return m_state->MoveUp; }
    [[nodiscard]] bool IsMoveDownPressed() const noexcept { return m_state->MoveDown; }
    [[nodiscard]] bool IsLookLeftPressed() const noexcept { return m_state->LookLeft; }
    [[nodiscard]] bool IsLookRightPressed() const noexcept { return m_state->LookRight; }
    [[nodiscard]] bool IsLookUpPressed() const noexcept { return m_state->LookUp; }
    [[nodiscard]] bool IsLookDownPressed() const noexcept { return m_state->LookDown; }
    [[nodiscard]] bool IsFastMovePressed() const noexcept { return m_state->FastMove; }
    [[nodiscard]] bool IsRightMouseButtonDown() const noexcept { return m_state->RightMouseButtonDown; }
    [[nodiscard]] int GetMouseDeltaX() const noexcept { return m_state->MouseDeltaX; }
    [[nodiscard]] int GetMouseDeltaY() const noexcept { return m_state->MouseDeltaY; }

private:
    const Pragma::Platform::InputState* m_state = nullptr;
};
}
