#pragma once

#include "Pragma/Renderer/Entity.h"
#include "Pragma/Renderer/EntityHandle.h"

#include <cstddef>
#include <string_view>
#include <vector>

namespace Pragma::Renderer
{
class Scene;

class World
{
public:
    World() = default;
    explicit World(Scene& scene) noexcept
        : m_scene(&scene)
    {
    }

    [[nodiscard]] bool IsValid() const noexcept;
    [[nodiscard]] EntityHandle GetEntity(EntityId id) const noexcept;
    [[nodiscard]] EntityHandle FindEntityByName(std::string_view name) const noexcept;
    [[nodiscard]] EntityHandle GetActiveCameraEntity() const noexcept;
    [[nodiscard]] std::vector<EntityHandle> GetRootEntities() const;
    [[nodiscard]] std::vector<EntityHandle> GetChildren(EntityId parentId) const;
    [[nodiscard]] std::size_t GetEntityCount() const noexcept;

private:
    Scene* m_scene = nullptr;
};
}
