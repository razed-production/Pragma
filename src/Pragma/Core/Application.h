#pragma once

#include "Pragma/Core/EngineConfig.h"

namespace Pragma::Renderer
{
class RenderSystem;
}

namespace Pragma::RHI
{
class IDevice;
}

namespace Pragma::Platform
{
class Window;
}

namespace Pragma::Core
{
class Application
{
public:
    Application();
    ~Application();

    void Run();

private:
    EngineConfig m_config;
};
}
