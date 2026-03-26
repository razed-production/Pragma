#pragma once

#include "Pragma/Core/EngineInput.h"
#include "Pragma/Core/EngineTime.h"
#include "Pragma/Renderer/Entity.h"

namespace Pragma::Renderer
{
class Scene;

struct BehaviourContext
{
    Scene& World;
    EntityId Entity = InvalidEntityId;
    const Pragma::Core::EngineTime& Time;
    const Pragma::Core::EngineInput& Input;
};
}
