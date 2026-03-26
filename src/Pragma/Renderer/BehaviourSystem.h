#pragma once

#include "Pragma/Core/EngineInput.h"
#include "Pragma/Core/EngineTime.h"

namespace Pragma::Renderer
{
class Scene;

class BehaviourSystem
{
public:
    void Initialize(Scene& scene) noexcept;
    void Update(Scene& scene, const Pragma::Core::EngineTime& time, const Pragma::Core::EngineInput& input);
    void Shutdown(Scene& scene, const Pragma::Core::EngineTime& time, const Pragma::Core::EngineInput& input);
};
}
