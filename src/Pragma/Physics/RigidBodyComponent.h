#pragma once

#include "Pragma/Renderer/Component.h"

namespace Pragma::Renderer
{
enum class RigidBodyMotionType
{
    Static,
    Dynamic,
    Kinematic
};

enum class RigidBodyCollisionLayer
{
    Default,
    NoCollision
};

struct RigidBodyComponent final : public SceneComponent
{
    static constexpr ComponentType kType = ComponentType::RigidBody;
    static constexpr const char* kName = "Rigid Body";

    bool Enabled = true;
    RigidBodyMotionType MotionType = RigidBodyMotionType::Dynamic;
    RigidBodyCollisionLayer CollisionLayer = RigidBodyCollisionLayer::Default;
    float Friction = 0.5f;
    float Restitution = 0.0f;
    float LinearDamping = 0.05f;
    float AngularDamping = 0.05f;
    float GravityFactor = 1.0f;

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
