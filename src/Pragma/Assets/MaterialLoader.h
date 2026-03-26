#pragma once

#include "Pragma/Assets/MaterialAssetData.h"

#include <filesystem>

namespace Pragma::Assets
{
[[nodiscard]] MaterialAssetData LoadMaterialAsset(const std::filesystem::path& path);
void SaveMaterialAsset(const MaterialAssetData& material, const std::filesystem::path& path);
}
