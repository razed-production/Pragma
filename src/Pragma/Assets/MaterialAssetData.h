#pragma once

#include "Pragma/Assets/AssetId.h"

namespace Pragma::Assets
{
struct MaterialAssetData
{
    float BaseColor[4]{ 1.0f, 1.0f, 1.0f, 1.0f };
    float EmissiveColor[3]{ 0.0f, 0.0f, 0.0f };
    float Roughness = 0.5f;
    float Metallic = 0.0f;
    float AmbientOcclusion = 1.0f;
    bool UseAlbedoTexture = false;
    bool UseNormalTexture = false;
    bool UseOrmTexture = false;
    bool UseEmissiveTexture = false;
    float EmissiveIntensity = 0.0f;
    float NormalStrength = 1.0f;
    AssetId AlbedoTextureAsset;
    AssetId NormalTextureAsset;
    AssetId OrmTextureAsset;
    AssetId EmissiveTextureAsset;
};
}
