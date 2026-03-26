#include "Pragma/Renderer/BehaviourSystem.h"

#include "Pragma/Core/Profiler.h"
#include "Pragma/Renderer/BehaviourContext.h"
#include "Pragma/Renderer/Scene.h"
#include "Pragma/Renderer/SceneObject.h"

namespace Pragma::Renderer
{
void BehaviourSystem::Initialize(Scene& scene) noexcept
{
    (void)scene;
}

void BehaviourSystem::Update(Scene& scene, const Pragma::Core::EngineTime& time, const Pragma::Core::EngineInput& input)
{
    PRAGMA_PROFILE_SCOPE("BehaviourSystem::Update");
    for (SceneObject& object : scene.GetObjects())
    {
        BehaviourComponent* behaviour = object.GetBehaviour();
        if (behaviour == nullptr || !behaviour->IsEnabled())
        {
            continue;
        }

        const BehaviourContext context
        {
            scene,
            object.Id,
            time,
            input
        };

        if (!behaviour->HasStarted())
        {
            behaviour->OnStart(context);
            behaviour->MarkStarted();
        }

        behaviour->OnUpdate(context);
    }
}

void BehaviourSystem::Shutdown(Scene& scene, const Pragma::Core::EngineTime& time, const Pragma::Core::EngineInput& input)
{
    PRAGMA_PROFILE_SCOPE("BehaviourSystem::Shutdown");
    for (SceneObject& object : scene.GetObjects())
    {
        BehaviourComponent* behaviour = object.GetBehaviour();
        if (behaviour == nullptr || !behaviour->HasStarted())
        {
            continue;
        }

        const BehaviourContext context
        {
            scene,
            object.Id,
            time,
            input
        };
        behaviour->OnDestroy(context);
    }
}
}
