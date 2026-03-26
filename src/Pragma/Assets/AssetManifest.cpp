#include "Pragma/Assets/AssetManifest.h"

#include "Pragma/Core/Log.h"

#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace Pragma::Assets
{
AssetManifest AssetManifest::LoadFromFile(const std::filesystem::path& path)
{
    std::ifstream input(path);
    if (!input.is_open())
    {
        throw std::runtime_error("Failed to open asset manifest.");
    }

    Pragma::Core::Log(Pragma::Core::LogLevel::Info, "Loading asset manifest: " + path.string());

    AssetManifest manifest;
    manifest.m_sourcePath = path;
    manifest.m_rootPath = path.parent_path();
    std::string line;

    while (std::getline(input, line))
    {
        if (line.empty() || line[0] == '#')
        {
            continue;
        }

        const std::size_t separatorIndex = line.find('=');
        if (separatorIndex == std::string::npos)
        {
            continue;
        }

        const std::string key = line.substr(0, separatorIndex);
        const std::string value = line.substr(separatorIndex + 1);
        manifest.m_entries.emplace(key, std::filesystem::weakly_canonical(manifest.m_rootPath / value));
    }

    return manifest;
}

std::filesystem::path AssetManifest::ResolvePath(const AssetId& assetId) const
{
    const auto it = m_entries.find(assetId.Value);
    if (it == m_entries.end())
    {
        throw std::runtime_error("Asset id is missing in the asset manifest.");
    }

    return it->second;
}

std::vector<AssetId> AssetManifest::FindAssetIdsByPrefix(const std::string& prefix) const
{
    std::vector<AssetId> assetIds;
    for (const auto& [key, _] : m_entries)
    {
        if (key.rfind(prefix, 0) == 0)
        {
            assetIds.push_back({ key });
        }
    }

    return assetIds;
}

void AssetManifest::RegisterAsset(const AssetId& assetId, const std::filesystem::path& relativePath)
{
    if (assetId.empty() || relativePath.empty())
    {
        throw std::runtime_error("AssetManifest::RegisterAsset requires a valid asset id and relative path.");
    }

    if (m_rootPath.empty())
    {
        throw std::runtime_error("AssetManifest::RegisterAsset requires a loaded manifest root path.");
    }

    m_entries[assetId.Value] = std::filesystem::weakly_canonical(m_rootPath / relativePath);
}

void AssetManifest::Save() const
{
    if (m_sourcePath.empty() || m_rootPath.empty())
    {
        throw std::runtime_error("AssetManifest::Save requires a loaded source path.");
    }

    std::ofstream output(m_sourcePath, std::ios::trunc);
    if (!output.is_open())
    {
        throw std::runtime_error("Failed to open asset manifest for writing.");
    }

    output << "# Pragma asset manifest\n";

    std::map<std::string, std::filesystem::path> sortedEntries(m_entries.begin(), m_entries.end());
    for (const auto& [key, absolutePath] : sortedEntries)
    {
        std::filesystem::path relativePath = std::filesystem::relative(absolutePath, m_rootPath);
        output << key << '=' << relativePath.generic_string() << '\n';
    }
}
}
