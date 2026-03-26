#pragma once

#include "Pragma/Renderer/BehaviourComponent.h"
#include "Pragma/Renderer/CameraComponent.h"
#include "Pragma/Renderer/CameraControllerComponent.h"
#include "Pragma/Renderer/Component.h"
#include "Pragma/Renderer/Entity.h"
#include "Pragma/Renderer/LightComponent.h"
#include "Pragma/Renderer/ManagedScriptComponent.h"
#include "Pragma/Renderer/MeshRendererComponent.h"
#include "Pragma/Renderer/PrefabInstanceComponent.h"
#include "Pragma/Physics/BoxColliderComponent.h"
#include "Pragma/Physics/RigidBodyComponent.h"
#include "Pragma/Renderer/Transform.h"

#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace Pragma::Renderer
{
struct SceneObject
{
    EntityId Id = InvalidEntityId;
    EntityId ParentId = InvalidEntityId;
    std::string Name;
    Transform Transform;
    std::array<std::shared_ptr<SceneComponent>, kComponentTypeCount> ComponentSlots{};
    std::vector<ComponentType> ComponentOrder;

    [[nodiscard]] Pragma::Renderer::Transform& GetTransform() noexcept
    {
        return Transform;
    }

    [[nodiscard]] const Pragma::Renderer::Transform& GetTransform() const noexcept
    {
        return Transform;
    }

    [[nodiscard]] bool HasCamera() const noexcept
    {
        return HasComponent<CameraComponent>();
    }

    [[nodiscard]] CameraComponent* GetCamera() noexcept
    {
        return GetComponent<CameraComponent>();
    }

    [[nodiscard]] const CameraComponent* GetCamera() const noexcept
    {
        return GetComponent<CameraComponent>();
    }

    [[nodiscard]] bool HasCameraController() const noexcept
    {
        return HasComponent<CameraControllerComponent>();
    }

    [[nodiscard]] CameraControllerComponent* GetCameraController() noexcept
    {
        return GetComponent<CameraControllerComponent>();
    }

    [[nodiscard]] const CameraControllerComponent* GetCameraController() const noexcept
    {
        return GetComponent<CameraControllerComponent>();
    }

    [[nodiscard]] bool HasMeshRenderer() const noexcept
    {
        return HasComponent<MeshRendererComponent>();
    }

    [[nodiscard]] MeshRendererComponent* GetMeshRenderer() noexcept
    {
        return GetComponent<MeshRendererComponent>();
    }

    [[nodiscard]] bool HasLight() const noexcept
    {
        return HasComponent<LightComponent>();
    }

    [[nodiscard]] LightComponent* GetLight() noexcept
    {
        return GetComponent<LightComponent>();
    }

    [[nodiscard]] const LightComponent* GetLight() const noexcept
    {
        return GetComponent<LightComponent>();
    }

    [[nodiscard]] bool HasRigidBody() const noexcept
    {
        return HasComponent<RigidBodyComponent>();
    }

    [[nodiscard]] RigidBodyComponent* GetRigidBody() noexcept
    {
        return GetComponent<RigidBodyComponent>();
    }

    [[nodiscard]] const RigidBodyComponent* GetRigidBody() const noexcept
    {
        return GetComponent<RigidBodyComponent>();
    }

    [[nodiscard]] bool HasBoxCollider() const noexcept
    {
        return HasComponent<BoxColliderComponent>();
    }

    [[nodiscard]] BoxColliderComponent* GetBoxCollider() noexcept
    {
        return GetComponent<BoxColliderComponent>();
    }

    [[nodiscard]] const BoxColliderComponent* GetBoxCollider() const noexcept
    {
        return GetComponent<BoxColliderComponent>();
    }

    [[nodiscard]] const MeshRendererComponent* GetMeshRenderer() const noexcept
    {
        return GetComponent<MeshRendererComponent>();
    }

    [[nodiscard]] bool HasPrefabInstance() const noexcept
    {
        return HasComponent<PrefabInstanceComponent>();
    }

    [[nodiscard]] PrefabInstanceComponent* GetPrefabInstance() noexcept
    {
        return GetComponent<PrefabInstanceComponent>();
    }

    [[nodiscard]] const PrefabInstanceComponent* GetPrefabInstance() const noexcept
    {
        return GetComponent<PrefabInstanceComponent>();
    }

    [[nodiscard]] bool HasBehaviour() const noexcept
    {
        return GetBehaviour() != nullptr;
    }

    [[nodiscard]] BehaviourComponent* GetBehaviour() noexcept
    {
        if (auto* managedScript = GetComponent<ManagedScriptComponent>(); managedScript != nullptr)
        {
            return managedScript;
        }

        return GetComponent<BehaviourComponent>();
    }

    [[nodiscard]] const BehaviourComponent* GetBehaviour() const noexcept
    {
        if (const auto* managedScript = GetComponent<ManagedScriptComponent>(); managedScript != nullptr)
        {
            return managedScript;
        }

        return GetComponent<BehaviourComponent>();
    }

    [[nodiscard]] bool HasManagedScript() const noexcept
    {
        return HasComponent<ManagedScriptComponent>();
    }

    [[nodiscard]] ManagedScriptComponent* GetManagedScript() noexcept
    {
        return GetComponent<ManagedScriptComponent>();
    }

    [[nodiscard]] const ManagedScriptComponent* GetManagedScript() const noexcept
    {
        return GetComponent<ManagedScriptComponent>();
    }

    template <typename TComponent, typename... TArgs>
    TComponent& AddComponent(TArgs&&... args)
    {
        static_assert(std::is_base_of_v<SceneComponent, TComponent>, "TComponent must derive from SceneComponent.");

        if (TComponent* existingComponent = GetComponent<TComponent>())
        {
            return *existingComponent;
        }

        auto component = std::make_shared<TComponent>(std::forward<TArgs>(args)...);
        return AddComponent<TComponent>(std::move(component));
    }

    template <typename TComponent>
    TComponent& AddComponent(std::shared_ptr<TComponent> component)
    {
        static_assert(std::is_base_of_v<SceneComponent, TComponent>, "TComponent must derive from SceneComponent.");

        TComponent* existingComponent = GetComponent<TComponent>();
        if (existingComponent != nullptr)
        {
            return *existingComponent;
        }

        const std::size_t componentIndex = ToComponentIndex(TComponent::kType);
        TComponent& componentRef = *component;
        ComponentSlots[componentIndex] = std::move(component);
        ComponentOrder.push_back(TComponent::kType);
        return componentRef;
    }

    template <typename TComponent>
    [[nodiscard]] bool HasComponent() const noexcept
    {
        return GetComponent<TComponent>() != nullptr;
    }

    template <typename TComponent>
    [[nodiscard]] TComponent* GetComponent() noexcept
    {
        static_assert(std::is_base_of_v<SceneComponent, TComponent>, "TComponent must derive from SceneComponent.");
        return static_cast<TComponent*>(GetComponentByType(TComponent::kType));
    }

    template <typename TComponent>
    [[nodiscard]] const TComponent* GetComponent() const noexcept
    {
        static_assert(std::is_base_of_v<SceneComponent, TComponent>, "TComponent must derive from SceneComponent.");
        return static_cast<const TComponent*>(GetComponentByType(TComponent::kType));
    }

    template <typename TComponent>
    bool RemoveComponent() noexcept
    {
        static_assert(std::is_base_of_v<SceneComponent, TComponent>, "TComponent must derive from SceneComponent.");
        const std::size_t componentIndex = ToComponentIndex(TComponent::kType);
        const bool removed = ComponentSlots[componentIndex] != nullptr;
        ComponentSlots[componentIndex].reset();
        ComponentOrder.erase(
            std::remove(ComponentOrder.begin(), ComponentOrder.end(), TComponent::kType),
            ComponentOrder.end());
        return removed;
    }

    [[nodiscard]] SceneComponent* GetComponentByType(const ComponentType type) noexcept
    {
        return ComponentSlots[ToComponentIndex(type)].get();
    }

    [[nodiscard]] const SceneComponent* GetComponentByType(const ComponentType type) const noexcept
    {
        return ComponentSlots[ToComponentIndex(type)].get();
    }

    [[nodiscard]] const std::vector<ComponentType>& GetComponentOrder() const noexcept
    {
        return ComponentOrder;
    }
};
}
