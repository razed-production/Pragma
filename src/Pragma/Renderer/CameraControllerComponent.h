#pragma once

#include "Pragma/Renderer/Component.h"

namespace Pragma::Renderer
{
struct CameraControllerComponent final : public SceneComponent
{
    static constexpr ComponentType kType = ComponentType::CameraController;
    static constexpr const char* kName = "Camera Controller";

    float MoveSpeed = 4.0f;
    float FastMoveSpeed = 8.0f;
    float KeyboardLookSpeed = 1.6f;
    float MouseLookSensitivity = 0.0035f;
    bool Enabled = true;

    [[nodiscard]] ComponentType GetComponentType() const noexcept override
    {
        return kType;
    }

    [[nodiscard]] const char* GetComponentName() const noexcept override
    {
        return kName;
    }
};
}
