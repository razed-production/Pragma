#pragma once

#include "Pragma/Math/Vector3.h"
#include "Pragma/Renderer/Component.h"

namespace Pragma::Renderer
{
struct BoxColliderComponent final : public SceneComponent
{
    static constexpr ComponentType kType = ComponentType::BoxCollider;
    static constexpr const char* kName = "Box Collider";

    Pragma::Math::Vector3 HalfExtent{ 0.5f, 0.5f, 0.5f };

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
