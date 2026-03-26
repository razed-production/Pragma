#pragma once

#include "Pragma/Renderer/Vertex.h"
#include "Pragma/Math/Vector3.h"

#include <cstdint>
#include <vector>

namespace Pragma::Assets
{
struct MeshAssetData
{
    std::vector<Pragma::Renderer::VertexPCN> Vertices;
    std::vector<std::uint32_t> Indices;
    Pragma::Math::Vector3 LocalBoundsCenter{};
    float LocalBoundsRadius = 0.0f;
};
}
