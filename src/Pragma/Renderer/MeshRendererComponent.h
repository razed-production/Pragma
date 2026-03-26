#pragma once

#include "Pragma/Renderer/Component.h"
#include "Pragma/Assets/AssetId.h"
#include "Pragma/Renderer/Material.h"
#include "Pragma/Renderer/Mesh.h"

#include <memory>

namespace Pragma::Renderer
{
enum class MeshLodSlot
{
    Base,
    Lod1,
    Lod2
};

struct MeshRendererComponent final : public SceneComponent
{
    static constexpr ComponentType kType = ComponentType::MeshRenderer;
    static constexpr const char* kName = "Mesh Renderer";

    Pragma::Assets::AssetId MeshAssetId;
    Pragma::Assets::AssetId MediumLodMeshAssetId;
    Pragma::Assets::AssetId LowLodMeshAssetId;
    std::shared_ptr<Pragma::Renderer::Mesh> Mesh;
    std::shared_ptr<Pragma::Renderer::Mesh> MediumLodMesh;
    std::shared_ptr<Pragma::Renderer::Mesh> LowLodMesh;
    Pragma::Assets::AssetId MaterialAssetId;
    std::shared_ptr<Pragma::Renderer::Material> Material;

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
