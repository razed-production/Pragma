#pragma once

#include "Pragma/RHI/Resources.h"

#include <memory>

namespace Pragma::Renderer
{
struct MaterialParameters
{
    float BaseColor[4]{ 1.0f, 1.0f, 1.0f, 1.0f };
    float EmissiveColor[3]{ 0.0f, 0.0f, 0.0f };
    float Roughness = 0.5f;
    float Metallic = 0.0f;
    float AmbientOcclusion = 1.0f;
    float UseAlbedoTexture = 0.0f;
    float EmissiveIntensity = 0.0f;
    float UseNormalTexture = 0.0f;
    float UseOrmTexture = 0.0f;
    float UseEmissiveTexture = 0.0f;
    float NormalStrength = 1.0f;
};

struct Material
{
    std::unique_ptr<Pragma::RHI::IPipelineState> Pipeline;
    std::unique_ptr<Pragma::RHI::IBuffer> ParametersBuffer;
    std::shared_ptr<Pragma::RHI::ITexture> AlbedoTexture;
    std::shared_ptr<Pragma::RHI::ITexture> NormalTexture;
    std::shared_ptr<Pragma::RHI::ITexture> OrmTexture;
    std::shared_ptr<Pragma::RHI::ITexture> EmissiveTexture;
    MaterialParameters Parameters;
};
}
