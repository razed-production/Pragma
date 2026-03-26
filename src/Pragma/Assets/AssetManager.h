#pragma once

#include "Pragma/Assets/AssetManifest.h"
#include "Pragma/Assets/ImageAssetData.h"
#include "Pragma/Assets/MaterialAssetData.h"
#include "Pragma/Assets/MeshAssetData.h"
#include "Pragma/Renderer/Material.h"
#include "Pragma/Renderer/Mesh.h"

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

namespace Pragma::RHI
{
class IDevice;
class ITexture;
}

namespace Pragma::Assets
{
enum class TextureColorSpace
{
    Color,
    LinearData
};

class AssetManager
{
public:
    AssetManager(Pragma::RHI::IDevice& device, AssetManifest manifest);

    [[nodiscard]] std::filesystem::path ResolvePath(const AssetId& assetId) const;
    [[nodiscard]] std::vector<AssetId> GetAssetIdsByPrefix(const std::string& prefix) const;
    [[nodiscard]] MaterialAssetData GetMaterialAssetData(const AssetId& assetId) const;
    [[nodiscard]] bool SaveMaterialAssetData(const AssetId& assetId, const MaterialAssetData& assetData);
    bool RegisterAsset(const AssetId& assetId, const std::filesystem::path& relativePath);
    [[nodiscard]] std::shared_ptr<Pragma::Renderer::Mesh> LoadMesh(const AssetId& assetId);
    [[nodiscard]] std::shared_ptr<Pragma::Renderer::Material> LoadMaterial(const AssetId& assetId);
    [[nodiscard]] std::shared_ptr<Pragma::RHI::ITexture> LoadTexture(
        const AssetId& assetId,
        TextureColorSpace colorSpace = TextureColorSpace::Color);

private:
    [[nodiscard]] std::shared_ptr<Pragma::Renderer::Mesh> UploadMesh(const MeshAssetData& assetData);
    [[nodiscard]] std::shared_ptr<Pragma::Renderer::Material> UploadMaterial(const MaterialAssetData& assetData);
    [[nodiscard]] std::shared_ptr<Pragma::RHI::ITexture> UploadTexture(
        const ImageAssetData& assetData,
        TextureColorSpace colorSpace);

private:
    Pragma::RHI::IDevice& m_device;
    AssetManifest m_manifest;
    std::unordered_map<std::string, std::weak_ptr<Pragma::Renderer::Mesh>> m_meshCache;
    std::unordered_map<std::string, std::weak_ptr<Pragma::Renderer::Material>> m_materialCache;
    std::unordered_map<std::string, std::weak_ptr<Pragma::RHI::ITexture>> m_textureCache;
};
}
