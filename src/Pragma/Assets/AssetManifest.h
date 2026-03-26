#pragma once

#include "Pragma/Assets/AssetId.h"

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace Pragma::Assets
{
class AssetManifest
{
public:
    static AssetManifest LoadFromFile(const std::filesystem::path& path);

    [[nodiscard]] std::filesystem::path ResolvePath(const AssetId& assetId) const;
    [[nodiscard]] std::vector<AssetId> FindAssetIdsByPrefix(const std::string& prefix) const;
    void RegisterAsset(const AssetId& assetId, const std::filesystem::path& relativePath);
    void Save() const;

private:
    std::filesystem::path m_sourcePath;
    std::filesystem::path m_rootPath;
    std::unordered_map<std::string, std::filesystem::path> m_entries;
};
}
