#pragma once

#include "Pragma/Assets/AssetId.h"

namespace Pragma::Assets
{
struct MaterialAssetData
{
    float BaseColor[4]{ 1.0f, 1.0f, 1.0f, 1.0f };
    float Roughness = 0.5f;
    bool UseAlbedoTexture = false;
    AssetId AlbedoTextureAsset;
};
}
