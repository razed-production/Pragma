#pragma once

#include "Pragma/Renderer/Material.h"
#include "Pragma/Renderer/Mesh.h"

#include <memory>

namespace Pragma::RHI
{
class IDevice;
}

namespace Pragma::Renderer
{
[[nodiscard]] std::shared_ptr<Mesh> CreateCubeMesh(Pragma::RHI::IDevice& device);
[[nodiscard]] std::shared_ptr<Material> CreateDefaultMaterial(Pragma::RHI::IDevice& device);
}
