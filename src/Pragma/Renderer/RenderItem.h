#pragma once

#include "Pragma/Renderer/Material.h"
#include "Pragma/Renderer/Mesh.h"
#include "Pragma/Renderer/Transform.h"

#include <memory>

namespace Pragma::Renderer
{
struct RenderItem
{
    std::shared_ptr<Mesh> Mesh;
    std::shared_ptr<Material> Material;
    Transform Transform;
};
}
