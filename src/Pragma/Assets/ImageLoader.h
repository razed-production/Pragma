#pragma once

#include "Pragma/Assets/ImageAssetData.h"
#include "Pragma/Core/Result.h"

#include <filesystem>

namespace Pragma::Assets
{
[[nodiscard]] Pragma::Core::Result<ImageAssetData> LoadPpmImage(const std::filesystem::path& path);
}
