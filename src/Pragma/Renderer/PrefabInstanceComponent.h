#pragma once

#include "Pragma/Assets/AssetId.h"
#include "Pragma/Renderer/Component.h"

#include <string>

namespace Pragma::Renderer
{
struct PrefabInstanceComponent final : public SceneComponent
{
    static constexpr ComponentType kType = ComponentType::PrefabInstance;

    PrefabInstanceComponent() = default;

    explicit PrefabInstanceComponent(Pragma::Assets::AssetId prefabAssetId) noexcept
        : PrefabAssetId(std::move(prefabAssetId))
    {
    }

    [[nodiscard]] ComponentType GetComponentType() const noexcept override
    {
        return kType;
    }

    [[nodiscard]] const char* GetComponentName() const noexcept override
    {
        return "Prefab Instance";
    }

    Pragma::Assets::AssetId PrefabAssetId;
};
}
