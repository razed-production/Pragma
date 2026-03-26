#pragma once

#include "Pragma/Core/EngineInput.h"
#include "Pragma/Core/EngineTime.h"
#include "Pragma/Renderer\EntityHandle.h"
#include "Pragma/Renderer\Scene.h"
#include "Pragma/Renderer\Transform.h"
#include "Pragma/Renderer\World.h"

#include <cstdint>
#include <string_view>
#include <vector>

namespace Pragma::Renderer
{
class NativeScriptComponent;

class ScriptableEntity
{
    friend class NativeScriptComponent;

public:
    virtual ~ScriptableEntity() = default;

    virtual void OnStart() {}
    virtual void OnUpdate() {}
    virtual void OnDestroy() {}

protected:
    [[nodiscard]] float GetDeltaSeconds() const noexcept
    {
        return m_time != nullptr ? m_time->DeltaSeconds : 0.0f;
    }

    [[nodiscard]] float GetElapsedSeconds() const noexcept
    {
        return m_time != nullptr ? m_time->ElapsedSeconds : 0.0f;
    }

    [[nodiscard]] std::uint64_t GetFrameIndex() const noexcept
    {
        return m_time != nullptr ? m_time->FrameIndex : 0;
    }

    [[nodiscard]] EntityHandle GetEntity() const noexcept
    {
        return m_entity;
    }

    [[nodiscard]] bool IsEntityValid() const noexcept
    {
        return m_entity.IsValid();
    }

    [[nodiscard]] const char* GetName() const noexcept
    {
        return m_entity.GetName();
    }

    [[nodiscard]] Transform GetWorldTransform() const noexcept
    {
        return m_entity.GetWorldTransform();
    }

    [[nodiscard]] Transform* GetTransform() noexcept
    {
        return m_entity.GetTransform();
    }

    [[nodiscard]] CameraComponent* GetCamera() noexcept
    {
        return m_entity.GetCamera();
    }

    [[nodiscard]] MeshRendererComponent* GetMeshRenderer() noexcept
    {
        return m_entity.GetMeshRenderer();
    }

    [[nodiscard]] LightComponent* GetLight() noexcept
    {
        return m_entity.GetLight();
    }

    [[nodiscard]] RigidBodyComponent* GetRigidBody() noexcept
    {
        return m_entity.GetRigidBody();
    }

    [[nodiscard]] BoxColliderComponent* GetBoxCollider() noexcept
    {
        return m_entity.GetBoxCollider();
    }

    [[nodiscard]] PrefabInstanceComponent* GetPrefabInstance() noexcept
    {
        return m_entity.GetPrefabInstance();
    }

    [[nodiscard]] bool HasCamera() const noexcept
    {
        return m_entity.HasCamera();
    }

    [[nodiscard]] bool HasMeshRenderer() const noexcept
    {
        return m_entity.HasMeshRenderer();
    }

    [[nodiscard]] bool HasLight() const noexcept
    {
        return m_entity.HasLight();
    }

    [[nodiscard]] bool HasRigidBody() const noexcept
    {
        return m_entity.HasRigidBody();
    }

    [[nodiscard]] bool HasBoxCollider() const noexcept
    {
        return m_entity.HasBoxCollider();
    }

    [[nodiscard]] bool HasPrefabInstance() const noexcept
    {
        return m_entity.HasPrefabInstance();
    }

    [[nodiscard]] bool IsMoveForwardPressed() const noexcept
    {
        return m_input != nullptr && m_input->IsMoveForwardPressed();
    }

    [[nodiscard]] bool IsMoveBackwardPressed() const noexcept
    {
        return m_input != nullptr && m_input->IsMoveBackwardPressed();
    }

    [[nodiscard]] bool IsMoveLeftPressed() const noexcept
    {
        return m_input != nullptr && m_input->IsMoveLeftPressed();
    }

    [[nodiscard]] bool IsMoveRightPressed() const noexcept
    {
        return m_input != nullptr && m_input->IsMoveRightPressed();
    }

    [[nodiscard]] bool IsMoveUpPressed() const noexcept
    {
        return m_input != nullptr && m_input->IsMoveUpPressed();
    }

    [[nodiscard]] bool IsMoveDownPressed() const noexcept
    {
        return m_input != nullptr && m_input->IsMoveDownPressed();
    }

    [[nodiscard]] bool IsLookLeftPressed() const noexcept
    {
        return m_input != nullptr && m_input->IsLookLeftPressed();
    }

    [[nodiscard]] bool IsLookRightPressed() const noexcept
    {
        return m_input != nullptr && m_input->IsLookRightPressed();
    }

    [[nodiscard]] bool IsLookUpPressed() const noexcept
    {
        return m_input != nullptr && m_input->IsLookUpPressed();
    }

    [[nodiscard]] bool IsLookDownPressed() const noexcept
    {
        return m_input != nullptr && m_input->IsLookDownPressed();
    }

    [[nodiscard]] bool IsFastMovePressed() const noexcept
    {
        return m_input != nullptr && m_input->IsFastMovePressed();
    }

    [[nodiscard]] bool IsRightMouseButtonDown() const noexcept
    {
        return m_input != nullptr && m_input->IsRightMouseButtonDown();
    }

    [[nodiscard]] int GetMouseDeltaX() const noexcept
    {
        return m_input != nullptr ? m_input->GetMouseDeltaX() : 0;
    }

    [[nodiscard]] int GetMouseDeltaY() const noexcept
    {
        return m_input != nullptr ? m_input->GetMouseDeltaY() : 0;
    }

    [[nodiscard]] EntityHandle GetParent() const noexcept
    {
        return m_entity.GetParent();
    }

    [[nodiscard]] EntityId GetParentId() const noexcept
    {
        return m_entity.GetParentId();
    }

    [[nodiscard]] Scene& GetScene() noexcept
    {
        return *m_scene;
    }

    [[nodiscard]] World GetWorld() noexcept
    {
        return m_scene != nullptr ? World(*m_scene) : World{};
    }

    [[nodiscard]] World GetWorld() const noexcept
    {
        return m_scene != nullptr ? World(*m_scene) : World{};
    }

    [[nodiscard]] EntityHandle FindEntityByName(const std::string_view name) noexcept
    {
        return GetWorld().FindEntityByName(name);
    }

    [[nodiscard]] EntityHandle FindEntityByName(const std::string_view name) const noexcept
    {
        return GetWorld().FindEntityByName(name);
    }

    [[nodiscard]] EntityHandle GetActiveCameraEntity() noexcept
    {
        return GetWorld().GetActiveCameraEntity();
    }

    [[nodiscard]] EntityHandle GetActiveCameraEntity() const noexcept
    {
        return GetWorld().GetActiveCameraEntity();
    }

    [[nodiscard]] std::vector<EntityHandle> GetChildren() const
    {
        return m_entity.GetChildren();
    }

    [[nodiscard]] EntityHandle FindChildByName(const std::string_view name) const noexcept
    {
        return m_entity.FindChildByName(name);
    }

    [[nodiscard]] bool IsRootEntity() const noexcept
    {
        return m_entity.IsRoot();
    }

    [[nodiscard]] const Pragma::Core::EngineTime& GetTime() const noexcept
    {
        return *m_time;
    }

    [[nodiscard]] const Pragma::Core::EngineInput& GetInput() const noexcept
    {
        return *m_input;
    }

private:
    void Bind(Scene& scene, const EntityId entityId, const Pragma::Core::EngineTime& time, const Pragma::Core::EngineInput& input) noexcept
    {
        m_scene = &scene;
        m_entity = EntityHandle(scene, entityId);
        m_time = &time;
        m_input = &input;
    }

    void Unbind() noexcept
    {
        m_scene = nullptr;
        m_entity = {};
        m_time = nullptr;
        m_input = nullptr;
    }

private:
    Scene* m_scene = nullptr;
    EntityHandle m_entity;
    const Pragma::Core::EngineTime* m_time = nullptr;
    const Pragma::Core::EngineInput* m_input = nullptr;
};
}
