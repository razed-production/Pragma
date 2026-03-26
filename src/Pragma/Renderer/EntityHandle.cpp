#include "Pragma/Renderer/EntityHandle.h"

#include "Pragma/Renderer/Scene.h"

namespace Pragma::Renderer
{
bool EntityHandle::IsValid() const noexcept
{
    return m_scene != nullptr && m_scene->IsEntityAlive(m_id);
}

bool EntityHandle::HasCamera() const noexcept
{
    return GetCamera() != nullptr;
}

bool EntityHandle::HasCameraController() const noexcept
{
    return GetCameraController() != nullptr;
}

bool EntityHandle::HasMeshRenderer() const noexcept
{
    return GetMeshRenderer() != nullptr;
}

bool EntityHandle::HasLight() const noexcept
{
    return GetLight() != nullptr;
}

bool EntityHandle::HasRigidBody() const noexcept
{
    return GetRigidBody() != nullptr;
}

bool EntityHandle::HasBoxCollider() const noexcept
{
    return GetBoxCollider() != nullptr;
}

bool EntityHandle::HasPrefabInstance() const noexcept
{
    return GetPrefabInstance() != nullptr;
}

bool EntityHandle::HasBehaviour() const noexcept
{
    return GetBehaviour() != nullptr;
}

Transform* EntityHandle::GetTransform() noexcept
{
    return m_scene != nullptr ? m_scene->GetTransform(m_id) : nullptr;
}

const Transform* EntityHandle::GetTransform() const noexcept
{
    return m_scene != nullptr ? m_scene->GetTransform(m_id) : nullptr;
}

Transform EntityHandle::GetWorldTransform() const noexcept
{
    return m_scene != nullptr ? m_scene->GetWorldTransform(m_id) : Transform{};
}

CameraComponent* EntityHandle::GetCamera() noexcept
{
    return m_scene != nullptr ? m_scene->GetCamera(m_id) : nullptr;
}

const CameraComponent* EntityHandle::GetCamera() const noexcept
{
    return m_scene != nullptr ? m_scene->GetCamera(m_id) : nullptr;
}

CameraControllerComponent* EntityHandle::GetCameraController() noexcept
{
    return m_scene != nullptr ? m_scene->GetCameraController(m_id) : nullptr;
}

const CameraControllerComponent* EntityHandle::GetCameraController() const noexcept
{
    return m_scene != nullptr ? m_scene->GetCameraController(m_id) : nullptr;
}

MeshRendererComponent* EntityHandle::GetMeshRenderer() noexcept
{
    return m_scene != nullptr ? m_scene->GetMeshRenderer(m_id) : nullptr;
}

const MeshRendererComponent* EntityHandle::GetMeshRenderer() const noexcept
{
    return m_scene != nullptr ? m_scene->GetMeshRenderer(m_id) : nullptr;
}

LightComponent* EntityHandle::GetLight() noexcept
{
    return m_scene != nullptr ? m_scene->GetLight(m_id) : nullptr;
}

const LightComponent* EntityHandle::GetLight() const noexcept
{
    return m_scene != nullptr ? m_scene->GetLight(m_id) : nullptr;
}

RigidBodyComponent* EntityHandle::GetRigidBody() noexcept
{
    return m_scene != nullptr ? m_scene->GetRigidBody(m_id) : nullptr;
}

const RigidBodyComponent* EntityHandle::GetRigidBody() const noexcept
{
    return m_scene != nullptr ? m_scene->GetRigidBody(m_id) : nullptr;
}

BoxColliderComponent* EntityHandle::GetBoxCollider() noexcept
{
    return m_scene != nullptr ? m_scene->GetBoxCollider(m_id) : nullptr;
}

const BoxColliderComponent* EntityHandle::GetBoxCollider() const noexcept
{
    return m_scene != nullptr ? m_scene->GetBoxCollider(m_id) : nullptr;
}

PrefabInstanceComponent* EntityHandle::GetPrefabInstance() noexcept
{
    if (m_scene == nullptr)
    {
        return nullptr;
    }

    SceneObject* object = m_scene->FindObject(m_id);
    return object != nullptr ? object->GetPrefabInstance() : nullptr;
}

const PrefabInstanceComponent* EntityHandle::GetPrefabInstance() const noexcept
{
    if (m_scene == nullptr)
    {
        return nullptr;
    }

    const SceneObject* object = m_scene->FindObject(m_id);
    return object != nullptr ? object->GetPrefabInstance() : nullptr;
}

EntityId EntityHandle::GetParentId() const noexcept
{
    return m_scene != nullptr ? m_scene->GetParentId(m_id) : InvalidEntityId;
}

EntityHandle EntityHandle::GetParent() const noexcept
{
    if (m_scene == nullptr)
    {
        return {};
    }

    const EntityId parentId = m_scene->GetParentId(m_id);
    return parentId != InvalidEntityId ? EntityHandle(*m_scene, parentId) : EntityHandle{};
}

bool EntityHandle::IsRoot() const noexcept
{
    return GetParentId() == InvalidEntityId;
}

std::vector<EntityHandle> EntityHandle::GetChildren() const
{
    std::vector<EntityHandle> children;
    if (m_scene == nullptr)
    {
        return children;
    }

    for (const EntityId childId : m_scene->GetChildren(m_id))
    {
        children.emplace_back(*m_scene, childId);
    }

    return children;
}

EntityHandle EntityHandle::FindChildByName(const std::string_view name) const noexcept
{
    if (m_scene == nullptr)
    {
        return {};
    }

    for (const EntityId childId : m_scene->GetChildren(m_id))
    {
        const SceneObject* object = m_scene->FindObject(childId);
        if (object != nullptr && object->Name == name)
        {
            return EntityHandle(*m_scene, childId);
        }
    }

    return {};
}

const char* EntityHandle::GetName() const noexcept
{
    if (m_scene == nullptr)
    {
        return "";
    }

    const SceneObject* object = m_scene->FindObject(m_id);
    return object != nullptr ? object->Name.c_str() : "";
}

BehaviourComponent* EntityHandle::GetBehaviour() noexcept
{
    return m_scene != nullptr ? m_scene->GetBehaviour(m_id) : nullptr;
}

const BehaviourComponent* EntityHandle::GetBehaviour() const noexcept
{
    return m_scene != nullptr ? m_scene->GetBehaviour(m_id) : nullptr;
}
}
