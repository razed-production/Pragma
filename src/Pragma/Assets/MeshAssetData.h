#pragma once

#include "Pragma/Renderer/Vertex.h"

#include <cstdint>
#include <vector>

namespace Pragma::Assets
{
struct MeshAssetData
{
    std::vector<Pragma::Renderer::VertexPCN> Vertices;
    std::vector<std::uint32_t> Indices;
};
}
