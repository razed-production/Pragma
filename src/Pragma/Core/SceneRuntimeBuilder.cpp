#include "Pragma/Core/SceneRuntimeBuilder.h"

#include "Pragma/Assets/AssetManager.h"
#include "Pragma/Core/Assert.h"
#include "Pragma/Core/Log.h"
#include "Pragma/Physics/BoxColliderComponent.h"
#include "Pragma/Physics/RigidBodyComponent.h"
#include "Pragma/Renderer/MeshRendererComponent.h"
#include "Pragma/Renderer/ManagedScriptComponent.h"
#include "Pragma/Renderer/NativeScriptComponent.h"
#include "Pragma/Renderer/NativeScriptRegistry.h"
#include "Pragma/Renderer/PrefabInstanceComponent.h"
#include "Pragma/Scripting/ManagedScriptHost.h"

namespace Pragma::Core
{
SceneRuntimeBuilder::SceneRuntimeBuilder(
    Pragma::Assets::AssetManager& assets,
    Pragma::Renderer::NativeScriptRegistry& scriptRegistry,
    Pragma::Scripting::ManagedScriptHost& managedScriptHost)
    : m_assets(assets)
    , m_scriptRegistry(scriptRegistry)
    , m_managedScriptHost(managedScriptHost)
{
}

void SceneRuntimeBuilder::Build(Pragma::Renderer::Scene& scene, const SerializedScene& serializedScene)
{
    scene.Clear();
    scene.SetManagedScriptHost(&m_managedScriptHost);

    for (const SerializedSceneObject& objectDesc : serializedScene.Objects)
    {
        Pragma::Renderer::SceneObject& object = scene.CreateObjectWithId(objectDesc.Id, objectDesc.Name, objectDesc.Transform);

        if (objectDesc.MeshRenderer.has_value())
        {
            auto meshRenderer = std::make_shared<Pragma::Renderer::MeshRendererComponent>();
            meshRenderer->MeshAssetId = objectDesc.MeshRenderer->MeshAsset;
            meshRenderer->MediumLodMeshAssetId = objectDesc.MeshRenderer->MediumLodMeshAsset;
            meshRenderer->LowLodMeshAssetId = objectDesc.MeshRenderer->LowLodMeshAsset;
            meshRenderer->Mesh = m_assets.LoadMesh(objectDesc.MeshRenderer->MeshAsset);
            if (!objectDesc.MeshRenderer->MediumLodMeshAsset.empty())
            {
                meshRenderer->MediumLodMesh = m_assets.LoadMesh(objectDesc.MeshRenderer->MediumLodMeshAsset);
            }
            if (!objectDesc.MeshRenderer->LowLodMeshAsset.empty())
            {
                meshRenderer->LowLodMesh = m_assets.LoadMesh(objectDesc.MeshRenderer->LowLodMeshAsset);
            }
            meshRenderer->MaterialAssetId = objectDesc.MeshRenderer->MaterialAsset;
            meshRenderer->Material = m_assets.LoadMaterial(objectDesc.MeshRenderer->MaterialAsset);
            object.AddComponent(meshRenderer);
        }

        if (objectDesc.Camera.has_value())
        {
            object.AddComponent(std::make_shared<Pragma::Renderer::CameraComponent>(*objectDesc.Camera));
        }

        if (objectDesc.CameraController.has_value())
        {
            object.AddComponent(std::make_shared<Pragma::Renderer::CameraControllerComponent>(*objectDesc.CameraController));
        }

        if (objectDesc.Light.has_value())
        {
            object.AddComponent(std::make_shared<Pragma::Renderer::LightComponent>(*objectDesc.Light));
        }

        if (objectDesc.RigidBody.has_value())
        {
            object.AddComponent(std::make_shared<Pragma::Renderer::RigidBodyComponent>(*objectDesc.RigidBody));
        }

        if (objectDesc.BoxCollider.has_value())
        {
            object.AddComponent(std::make_shared<Pragma::Renderer::BoxColliderComponent>(*objectDesc.BoxCollider));
        }

        if (!objectDesc.PrefabAssetId.empty())
        {
            object.AddComponent(std::make_shared<Pragma::Renderer::PrefabInstanceComponent>(objectDesc.PrefabAssetId));
        }

        if (!objectDesc.ScriptName.empty())
        {
            std::shared_ptr<Pragma::Renderer::NativeScriptComponent> script = m_scriptRegistry.Create(objectDesc.ScriptName);
            PRAGMA_ASSERT(script != nullptr, "SceneRuntimeBuilder failed to create a registered native script.");
            object.AddComponent(script);
        }

        if (!objectDesc.ManagedScriptProjectAsset.empty())
        {
            object.AddComponent(std::make_shared<Pragma::Renderer::ManagedScriptComponent>(
                objectDesc.ManagedScriptProjectAsset,
                objectDesc.ManagedScriptTypeName));
        }

        if (objectDesc.IsActiveCamera)
        {
            const bool activeCameraSet = scene.SetActiveCamera(object.Id);
            PRAGMA_ASSERT(activeCameraSet, "SceneRuntimeBuilder failed to set the active camera.");
        }
    }

    for (const SerializedSceneObject& objectDesc : serializedScene.Objects)
    {
        if (objectDesc.ParentId != Pragma::Renderer::InvalidEntityId)
        {
            Pragma::Renderer::SceneObject* object = scene.FindObject(objectDesc.Id);
            const Pragma::Renderer::SceneObject* parent = scene.FindObject(objectDesc.ParentId);
            PRAGMA_ASSERT(object != nullptr, "SceneRuntimeBuilder failed to resolve child object while rebuilding hierarchy.");
            PRAGMA_ASSERT(parent != nullptr, "SceneRuntimeBuilder failed to resolve parent object while rebuilding hierarchy.");
            object->ParentId = objectDesc.ParentId;
        }
    }

    PRAGMA_ASSERT(!scene.GetObjects().empty(), "SceneRuntimeBuilder requires at least one scene object.");
    PRAGMA_ASSERT(scene.GetActiveCameraEntityId() != Pragma::Renderer::InvalidEntityId, "SceneRuntimeBuilder requires an active camera.");

    scene.Initialize();

    Pragma::Core::Log(
        Pragma::Core::LogCategory::Scene,
        Pragma::Core::LogLevel::Info,
        "SceneDocument loaded: " +
        std::to_string(scene.GetObjects().size()) + " objects, " +
        "active camera id " +
        std::to_string(scene.GetActiveCameraEntityId()) + '.');
}

}
