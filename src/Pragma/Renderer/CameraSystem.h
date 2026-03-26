#pragma once

#include "Pragma/Core/EngineInput.h"
#include "Pragma/Core/EngineTime.h"

namespace Pragma::Renderer
{
class Scene;

class CameraSystem
{
public:
    void Update(Scene& scene, const Pragma::Core::EngineTime& time, const Pragma::Core::EngineInput& input);
};
}
