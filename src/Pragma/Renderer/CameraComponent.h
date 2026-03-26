#pragma once

#include "Pragma/Renderer/Component.h"

namespace Pragma::Renderer
{
struct CameraComponent final : public SceneComponent
{
    static constexpr ComponentType kType = ComponentType::Camera;
    static constexpr const char* kName = "Camera";

    float PitchRadians = 0.0f;
    float FieldOfViewRadians = 1.0471975512f;
    float NearPlane = 0.1f;
    float FarPlane = 100.0f;

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
