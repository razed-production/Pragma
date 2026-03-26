#pragma once

#include "Pragma/RHI/Resources.h"
#include "Pragma/RHI/Types.h"

#include <memory>

namespace Pragma::Renderer
{
struct Mesh
{
    std::unique_ptr<Pragma::RHI::IBuffer> VertexBuffer;
    std::unique_ptr<Pragma::RHI::IBuffer> IndexBuffer;
    Pragma::RHI::IndexFormat IndexFormat = Pragma::RHI::IndexFormat::UInt32;
    std::uint32_t IndexCount = 0;
};
}
