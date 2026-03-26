#pragma once

#include "Pragma/Assets/MeshAssetData.h"

#include <filesystem>

namespace Pragma::Assets
{
[[nodiscard]] MeshAssetData LoadObjMesh(const std::filesystem::path& path);
}
