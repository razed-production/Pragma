#pragma once

#include "Pragma/Renderer/Entity.h"

#include <string_view>
#include <vector>

namespace Pragma::Renderer
{
class Scene;
struct Transform;
struct CameraComponent;
struct CameraControllerComponent;
struct MeshRendererComponent;
class LightComponent;
struct RigidBodyComponent;
struct BoxColliderComponent;
struct PrefabInstanceComponent;
class BehaviourComponent;

class EntityHandle
{
public:
    EntityHandle() = default;
    EntityHandle(Scene& scene, const EntityId id) noexcept
        : m_scene(&scene)
        , m_id(id)
    {
    }

    [[nodiscard]] EntityId GetId() const noexcept
    {
        return m_id;
    }

    [[nodiscard]] bool IsValid() const noexcept;
    [[nodiscard]] bool HasCamera() const noexcept;
    [[nodiscard]] bool HasCameraController() const noexcept;
    [[nodiscard]] bool HasMeshRenderer() const noexcept;
    [[nodiscard]] bool HasLight() const noexcept;
    [[nodiscard]] bool HasRigidBody() const noexcept;
    [[nodiscard]] bool HasBoxCollider() const noexcept;
    [[nodiscard]] bool HasPrefabInstance() const noexcept;
    [[nodiscard]] bool HasBehaviour() const noexcept;
    [[nodiscard]] Transform* GetTransform() noexcept;
    [[nodiscard]] const Transform* GetTransform() const noexcept;
    [[nodiscard]] Transform GetWorldTransform() const noexcept;
    [[nodiscard]] CameraComponent* GetCamera() noexcept;
    [[nodiscard]] const CameraComponent* GetCamera() const noexcept;
    [[nodiscard]] CameraControllerComponent* GetCameraController() noexcept;
    [[nodiscard]] const CameraControllerComponent* GetCameraController() const noexcept;
    [[nodiscard]] MeshRendererComponent* GetMeshRenderer() noexcept;
    [[nodiscard]] const MeshRendererComponent* GetMeshRenderer() const noexcept;
    [[nodiscard]] LightComponent* GetLight() noexcept;
    [[nodiscard]] const LightComponent* GetLight() const noexcept;
    [[nodiscard]] RigidBodyComponent* GetRigidBody() noexcept;
    [[nodiscard]] const RigidBodyComponent* GetRigidBody() const noexcept;
    [[nodiscard]] BoxColliderComponent* GetBoxCollider() noexcept;
    [[nodiscard]] const BoxColliderComponent* GetBoxCollider() const noexcept;
    [[nodiscard]] PrefabInstanceComponent* GetPrefabInstance() noexcept;
    [[nodiscard]] const PrefabInstanceComponent* GetPrefabInstance() const noexcept;
    [[nodiscard]] EntityId GetParentId() const noexcept;
    [[nodiscard]] EntityHandle GetParent() const noexcept;
    [[nodiscard]] bool IsRoot() const noexcept;
    [[nodiscard]] std::vector<EntityHandle> GetChildren() const;
    [[nodiscard]] EntityHandle FindChildByName(std::string_view name) const noexcept;
    [[nodiscard]] const char* GetName() const noexcept;
    [[nodiscard]] BehaviourComponent* GetBehaviour() noexcept;
    [[nodiscard]] const BehaviourComponent* GetBehaviour() const noexcept;

private:
    Scene* m_scene = nullptr;
    EntityId m_id = InvalidEntityId;
};
}
