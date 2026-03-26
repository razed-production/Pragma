#pragma once

#include "Pragma/RHI/Resources.h"

#include <memory>

namespace Pragma::Renderer
{
struct MaterialParameters
{
    float BaseColor[4]{ 1.0f, 1.0f, 1.0f, 1.0f };
    float Roughness = 0.5f;
    float UseAlbedoTexture = 0.0f;
    float Padding[2]{};
};

struct Material
{
    std::unique_ptr<Pragma::RHI::IPipelineState> Pipeline;
    std::unique_ptr<Pragma::RHI::IBuffer> ParametersBuffer;
    std::shared_ptr<Pragma::RHI::ITexture> AlbedoTexture;
    MaterialParameters Parameters;
};
}
