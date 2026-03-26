#include "Pragma/Renderer/World.h"

#include "Pragma/Renderer/Scene.h"

namespace Pragma::Renderer
{
bool World::IsValid() const noexcept
{
    return m_scene != nullptr;
}

EntityHandle World::GetEntity(const EntityId id) const noexcept
{
    return m_scene != nullptr ? m_scene->GetEntity(id) : EntityHandle{};
}

EntityHandle World::FindEntityByName(const std::string_view name) const noexcept
{
    return m_scene != nullptr ? m_scene->FindEntityByName(name) : EntityHandle{};
}

EntityHandle World::GetActiveCameraEntity() const noexcept
{
    return m_scene != nullptr ? m_scene->GetActiveCameraEntity() : EntityHandle{};
}

std::vector<EntityHandle> World::GetRootEntities() const
{
    std::vector<EntityHandle> entities;
    if (m_scene == nullptr)
    {
        return entities;
    }

    for (const SceneObject& object : m_scene->GetObjects())
    {
        if (object.ParentId == InvalidEntityId)
        {
            entities.emplace_back(*m_scene, object.Id);
        }
    }

    return entities;
}

std::vector<EntityHandle> World::GetChildren(const EntityId parentId) const
{
    std::vector<EntityHandle> entities;
    if (m_scene == nullptr)
    {
        return entities;
    }

    for (const EntityId childId : m_scene->GetChildren(parentId))
    {
        entities.emplace_back(*m_scene, childId);
    }

    return entities;
}

std::size_t World::GetEntityCount() const noexcept
{
    return m_scene != nullptr ? m_scene->GetObjects().size() : 0;
}
}
