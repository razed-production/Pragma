#pragma once

#include "Pragma/Core/SceneSerializer.h"

#include <filesystem>
#include <cstdint>
#include <vector>

namespace Pragma::Core
{
struct SerializedPrefab
{
    std::uint32_t Version = 1;
    std::vector<SerializedSceneObject> Objects;
};

[[nodiscard]] SerializedPrefab LoadPrefabFromFile(const std::filesystem::path& path);
void SavePrefabToFile(const SerializedPrefab& prefab, const std::filesystem::path& path);
}
