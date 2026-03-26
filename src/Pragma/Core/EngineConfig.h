#pragma once

#include "Pragma/RHI/BackendType.h"

namespace Pragma::Core
{
struct GraphicsConfig
{
    Pragma::RHI::BackendType Backend = Pragma::RHI::BackendType::Direct3D11;
};

struct EngineConfig
{
    GraphicsConfig Graphics;
};
}
