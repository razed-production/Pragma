#pragma once

#include "Pragma/Renderer/Component.h"
#include "Pragma/Assets/AssetId.h"
#include "Pragma/Renderer/Material.h"
#include "Pragma/Renderer/Mesh.h"

#include <memory>

namespace Pragma::Renderer
{
struct MeshRendererComponent final : public SceneComponent
{
    static constexpr ComponentType kType = ComponentType::MeshRenderer;
    static constexpr const char* kName = "Mesh Renderer";

    std::shared_ptr<Mesh> Mesh;
    Pragma::Assets::AssetId MaterialAssetId;
    std::shared_ptr<Material> Material;

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
