#pragma once

#include "Pragma/Core/EngineInput.h"
#include "Pragma/Core/EngineTime.h"
#include "Pragma/Renderer/BehaviourSystem.h"
#include "Pragma/Renderer/Component.h"
#include "Pragma/Renderer/Entity.h"
#include "Pragma/Renderer/EntityHandle.h"
#include "Pragma/Renderer/LightComponent.h"
#include "Pragma/Renderer/SceneObject.h"

#include <cstddef>
#include <stdexcept>
#include <string_view>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Pragma::Scripting
{
class ManagedScriptHost;
}

namespace Pragma::Renderer
{
class Scene
{
public:
    void SetManagedScriptHost(Pragma::Scripting::ManagedScriptHost* host) noexcept
    {
        m_managedScriptHost = host;
    }

    [[nodiscard]] Pragma::Scripting::ManagedScriptHost* GetManagedScriptHost() noexcept
    {
        return m_managedScriptHost;
    }

    [[nodiscard]] const Pragma::Scripting::ManagedScriptHost* GetManagedScriptHost() const noexcept
    {
        return m_managedScriptHost;
    }

    [[nodiscard]] SceneObject& CreateObject(std::string name, const Transform& transform = {})
    {
        return CreateObjectWithId(m_nextEntityId++, std::move(name), transform);
    }

    [[nodiscard]] SceneObject& CreateObjectWithId(const EntityId id, std::string name, const Transform& transform = {})
    {
        if (id == InvalidEntityId)
        {
            throw std::runtime_error("Scene requires a valid entity id.");
        }

        SceneObject& object = m_objects.emplace_back();
        object.Id = id;
        object.ParentId = InvalidEntityId;
        object.Name = std::move(name);
        object.Transform = transform;
        m_objectIndices.emplace(object.Id, m_objects.size() - 1);
        if (id >= m_nextEntityId)
        {
            m_nextEntityId = id + 1;
        }
        return object;
    }

    [[nodiscard]] SceneObject* DuplicateObject(const EntityId sourceId)
    {
        const SceneObject* source = FindObject(sourceId);
        if (source == nullptr)
        {
            return nullptr;
        }

        SceneObject& duplicate = CreateObject(source->Name + " Copy", source->GetTransform());
        duplicate.ParentId = source->ParentId;
        duplicate.ComponentSlots = source->ComponentSlots;
        duplicate.ComponentOrder = source->ComponentOrder;
        return &duplicate;
    }

    [[nodiscard]] bool DestroyObject(const EntityId id)
    {
        const auto indexIt = m_objectIndices.find(id);
        if (indexIt == m_objectIndices.end())
        {
            return false;
        }

        for (SceneObject& object : m_objects)
        {
            if (object.ParentId == id)
            {
                object.Transform = GetWorldTransform(object.Id);
                object.ParentId = InvalidEntityId;
            }
        }

        m_objects.erase(m_objects.begin() + static_cast<std::ptrdiff_t>(indexIt->second));
        RebuildObjectIndexMap();

        if (m_activeCameraEntityId == id)
        {
            m_activeCameraEntityId = InvalidEntityId;
            for (const SceneObject& object : m_objects)
            {
                if (object.HasCamera())
                {
                    m_activeCameraEntityId = object.Id;
                    break;
                }
            }
        }

        return true;
    }

    [[nodiscard]] bool SetParent(const EntityId childId, const EntityId parentId)
    {
        if (childId == InvalidEntityId)
        {
            return false;
        }

        SceneObject* child = FindObject(childId);
        if (child == nullptr)
        {
            return false;
        }

        if (parentId == childId)
        {
            return false;
        }

        if (parentId != InvalidEntityId)
        {
            const SceneObject* parent = FindObject(parentId);
            if (parent == nullptr || IsDescendant(parentId, childId))
            {
                return false;
            }
        }

        const Transform worldTransform = GetWorldTransform(childId);
        child->ParentId = parentId;
        child->Transform = parentId == InvalidEntityId
            ? worldTransform
            : MakeRelativeTransform(GetWorldTransform(parentId), worldTransform);
        return true;
    }

    [[nodiscard]] EntityId GetParentId(const EntityId id) const noexcept
    {
        const SceneObject* object = FindObject(id);
        return object != nullptr ? object->ParentId : InvalidEntityId;
    }

    [[nodiscard]] std::vector<EntityId> GetChildren(const EntityId parentId) const
    {
        std::vector<EntityId> children;
        for (const SceneObject& object : m_objects)
        {
            if (object.ParentId == parentId)
            {
                children.push_back(object.Id);
            }
        }

        return children;
    }

    [[nodiscard]] std::vector<EntityId> GetSubtree(const EntityId rootId) const
    {
        std::vector<EntityId> subtree;
        const SceneObject* object = FindObject(rootId);
        if (object == nullptr)
        {
            return subtree;
        }

        subtree.push_back(rootId);
        for (const EntityId childId : GetChildren(rootId))
        {
            const std::vector<EntityId> childSubtree = GetSubtree(childId);
            subtree.insert(subtree.end(), childSubtree.begin(), childSubtree.end());
        }

        return subtree;
    }

    [[nodiscard]] bool IsDescendant(const EntityId id, const EntityId potentialAncestorId) const noexcept
    {
        const SceneObject* object = FindObject(id);
        while (object != nullptr && object->ParentId != InvalidEntityId)
        {
            if (object->ParentId == potentialAncestorId)
            {
                return true;
            }

            object = FindObject(object->ParentId);
        }

        return false;
    }

    [[nodiscard]] Transform GetWorldTransform(const EntityId id) const noexcept
    {
        const SceneObject* object = FindObject(id);
        if (object == nullptr)
        {
            return {};
        }

        if (object->ParentId == InvalidEntityId)
        {
            return object->GetTransform();
        }

        return CombineTransforms(GetWorldTransform(object->ParentId), object->GetTransform());
    }

    [[nodiscard]] bool SetWorldTransform(const EntityId id, const Transform& worldTransform) noexcept
    {
        SceneObject* object = FindObject(id);
        if (object == nullptr)
        {
            return false;
        }

        object->Transform = object->ParentId == InvalidEntityId
            ? worldTransform
            : MakeRelativeTransform(GetWorldTransform(object->ParentId), worldTransform);
        return true;
    }

    void Clear() noexcept
    {
        m_objects.clear();
        m_objectIndices.clear();
        m_activeCameraEntityId = InvalidEntityId;
        m_nextEntityId = 1;
        m_frameIndex = 0;
        m_elapsedSeconds = 0.0f;
        m_initialized = false;
    }

    void RebuildObjectIndexMap() noexcept
    {
        m_objectIndices.clear();
        for (std::size_t i = 0; i < m_objects.size(); ++i)
        {
            m_objectIndices.emplace(m_objects[i].Id, i);
        }
    }

    void Initialize() noexcept
    {
        m_initialized = true;
        m_frameIndex = 0;
        m_elapsedSeconds = 0.0f;
        m_behaviourSystem.Initialize(*this);
    }

    void Shutdown(const Pragma::Core::EngineTime& time, const Pragma::Core::EngineInput& input)
    {
        m_behaviourSystem.Shutdown(*this, time, input);
        m_initialized = false;
    }

    void Update(const Pragma::Core::EngineTime& time, const Pragma::Core::EngineInput& input)
    {
        m_frameIndex = static_cast<std::size_t>(time.FrameIndex);
        m_elapsedSeconds = time.ElapsedSeconds;
        m_behaviourSystem.Update(*this, time, input);
    }

    [[nodiscard]] SceneObject* FindObject(const EntityId id) noexcept
    {
        const auto it = m_objectIndices.find(id);
        if (it == m_objectIndices.end())
        {
            return nullptr;
        }

        return &m_objects[it->second];
    }

    [[nodiscard]] const SceneObject* FindObject(const EntityId id) const noexcept
    {
        const auto it = m_objectIndices.find(id);
        if (it == m_objectIndices.end())
        {
            return nullptr;
        }

        return &m_objects[it->second];
    }

    [[nodiscard]] bool IsEntityAlive(const EntityId id) const noexcept
    {
        return FindObject(id) != nullptr;
    }

    [[nodiscard]] EntityHandle GetEntity(const EntityId id) noexcept
    {
        return EntityHandle(*this, id);
    }

    [[nodiscard]] EntityId FindEntityIdByName(const std::string_view name) const noexcept
    {
        for (const SceneObject& object : m_objects)
        {
            if (object.Name == name)
            {
                return object.Id;
            }
        }

        return InvalidEntityId;
    }

    [[nodiscard]] EntityHandle FindEntityByName(const std::string_view name) noexcept
    {
        const EntityId id = FindEntityIdByName(name);
        return id != InvalidEntityId ? EntityHandle(*this, id) : EntityHandle{};
    }

    template <typename TComponent>
    [[nodiscard]] TComponent* GetComponent(const EntityId id) noexcept
    {
        SceneObject* object = FindObject(id);
        return object != nullptr ? object->GetComponent<TComponent>() : nullptr;
    }

    template <typename TComponent>
    [[nodiscard]] const TComponent* GetComponent(const EntityId id) const noexcept
    {
        const SceneObject* object = FindObject(id);
        return object != nullptr ? object->GetComponent<TComponent>() : nullptr;
    }

    [[nodiscard]] Transform* GetTransform(const EntityId id) noexcept
    {
        SceneObject* object = FindObject(id);
        return object != nullptr ? &object->GetTransform() : nullptr;
    }

    [[nodiscard]] const Transform* GetTransform(const EntityId id) const noexcept
    {
        const SceneObject* object = FindObject(id);
        return object != nullptr ? &object->GetTransform() : nullptr;
    }

    [[nodiscard]] CameraComponent* GetCamera(const EntityId id) noexcept
    {
        return GetComponent<CameraComponent>(id);
    }

    [[nodiscard]] const CameraComponent* GetCamera(const EntityId id) const noexcept
    {
        return GetComponent<CameraComponent>(id);
    }

    [[nodiscard]] CameraControllerComponent* GetCameraController(const EntityId id) noexcept
    {
        return GetComponent<CameraControllerComponent>(id);
    }

    [[nodiscard]] const CameraControllerComponent* GetCameraController(const EntityId id) const noexcept
    {
        return GetComponent<CameraControllerComponent>(id);
    }

    [[nodiscard]] MeshRendererComponent* GetMeshRenderer(const EntityId id) noexcept
    {
        return GetComponent<MeshRendererComponent>(id);
    }

    [[nodiscard]] const MeshRendererComponent* GetMeshRenderer(const EntityId id) const noexcept
    {
        return GetComponent<MeshRendererComponent>(id);
    }

    [[nodiscard]] LightComponent* GetLight(const EntityId id) noexcept
    {
        return GetComponent<LightComponent>(id);
    }

    [[nodiscard]] const LightComponent* GetLight(const EntityId id) const noexcept
    {
        return GetComponent<LightComponent>(id);
    }

    [[nodiscard]] RigidBodyComponent* GetRigidBody(const EntityId id) noexcept
    {
        return GetComponent<RigidBodyComponent>(id);
    }

    [[nodiscard]] const RigidBodyComponent* GetRigidBody(const EntityId id) const noexcept
    {
        return GetComponent<RigidBodyComponent>(id);
    }

    [[nodiscard]] BoxColliderComponent* GetBoxCollider(const EntityId id) noexcept
    {
        return GetComponent<BoxColliderComponent>(id);
    }

    [[nodiscard]] const BoxColliderComponent* GetBoxCollider(const EntityId id) const noexcept
    {
        return GetComponent<BoxColliderComponent>(id);
    }

    [[nodiscard]] PrefabInstanceComponent* GetPrefabInstance(const EntityId id) noexcept
    {
        return GetComponent<PrefabInstanceComponent>(id);
    }

    [[nodiscard]] const PrefabInstanceComponent* GetPrefabInstance(const EntityId id) const noexcept
    {
        return GetComponent<PrefabInstanceComponent>(id);
    }

    [[nodiscard]] BehaviourComponent* GetBehaviour(const EntityId id) noexcept
    {
        return GetComponent<BehaviourComponent>(id);
    }

    [[nodiscard]] const BehaviourComponent* GetBehaviour(const EntityId id) const noexcept
    {
        return GetComponent<BehaviourComponent>(id);
    }

    [[nodiscard]] bool SetActiveCamera(const EntityId id) noexcept
    {
        if (id == InvalidEntityId)
        {
            m_activeCameraEntityId = InvalidEntityId;
            return true;
        }

        const SceneObject* object = FindObject(id);
        if (object == nullptr || !object->HasCamera())
        {
            return false;
        }

        m_activeCameraEntityId = id;
        return true;
    }

    [[nodiscard]] SceneObject* GetActiveCameraObject() noexcept
    {
        return FindObject(m_activeCameraEntityId);
    }

    [[nodiscard]] const SceneObject* GetActiveCameraObject() const noexcept
    {
        return FindObject(m_activeCameraEntityId);
    }

    [[nodiscard]] std::vector<SceneObject>& GetObjects() noexcept
    {
        return m_objects;
    }

    [[nodiscard]] const std::vector<SceneObject>& GetObjects() const noexcept
    {
        return m_objects;
    }

    [[nodiscard]] EntityId GetActiveCameraEntityId() const noexcept
    {
        return m_activeCameraEntityId;
    }

    [[nodiscard]] EntityHandle GetActiveCameraEntity() noexcept
    {
        return m_activeCameraEntityId != InvalidEntityId ? EntityHandle(*this, m_activeCameraEntityId) : EntityHandle{};
    }

    [[nodiscard]] bool IsInitialized() const noexcept
    {
        return m_initialized;
    }

    [[nodiscard]] std::size_t GetFrameIndex() const noexcept
    {
        return m_frameIndex;
    }

    [[nodiscard]] float GetElapsedSeconds() const noexcept
    {
        return m_elapsedSeconds;
    }

private:
    std::vector<SceneObject> m_objects;
    std::unordered_map<EntityId, std::size_t> m_objectIndices;
    BehaviourSystem m_behaviourSystem;
    Pragma::Scripting::ManagedScriptHost* m_managedScriptHost = nullptr;
    EntityId m_activeCameraEntityId = InvalidEntityId;
    EntityId m_nextEntityId = 1;
    std::size_t m_frameIndex = 0;
    float m_elapsedSeconds = 0.0f;
    bool m_initialized = false;
};
}
