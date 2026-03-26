#pragma once

#include <cstddef>

namespace Pragma::Renderer
{
enum class ComponentType
{
    Transform,
    Camera,
    CameraController,
    Light,
    MeshRenderer,
    Behaviour,
    ManagedScript,
    RigidBody,
    BoxCollider,
    PrefabInstance
};

constexpr std::size_t kComponentTypeCount = 10;

constexpr std::size_t ToComponentIndex(const ComponentType type) noexcept
{
    return static_cast<std::size_t>(type);
}

class SceneComponent
{
public:
    virtual ~SceneComponent() = default;

    [[nodiscard]] virtual ComponentType GetComponentType() const noexcept = 0;
    [[nodiscard]] virtual const char* GetComponentName() const noexcept = 0;
};
}
