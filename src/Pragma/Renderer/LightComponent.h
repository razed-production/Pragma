#pragma once

#include "Pragma/Renderer/Component.h"

namespace Pragma::Renderer
{
class LightComponent final : public SceneComponent
{
public:
    static constexpr ComponentType kType = ComponentType::Light;
    static constexpr const char* kName = "Directional Light";

    float Direction[3]{ -0.4f, -0.8f, 0.3f };
    float Intensity = 1.0f;
    float Color[3]{ 1.0f, 1.0f, 1.0f };
    float Padding0 = 0.0f;

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
