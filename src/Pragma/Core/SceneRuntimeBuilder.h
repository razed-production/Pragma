#pragma once

#include "Pragma/Core/SceneSerializer.h"
#include "Pragma/Renderer/Scene.h"

#include <memory>

namespace Pragma::Assets
{
class AssetManager;
}

namespace Pragma::RHI
{
}

namespace Pragma::Renderer
{
struct Material;
class NativeScriptRegistry;
}

namespace Pragma::Scripting
{
class ManagedScriptHost;
}

namespace Pragma::Core
{
class SceneRuntimeBuilder
{
public:
    SceneRuntimeBuilder(
        Pragma::Assets::AssetManager& assets,
        Pragma::Renderer::NativeScriptRegistry& scriptRegistry,
        Pragma::Scripting::ManagedScriptHost& managedScriptHost);

    void Build(Pragma::Renderer::Scene& scene, const SerializedScene& serializedScene);

private:
    Pragma::Assets::AssetManager& m_assets;
    Pragma::Renderer::NativeScriptRegistry& m_scriptRegistry;
    Pragma::Scripting::ManagedScriptHost& m_managedScriptHost;
};
}
