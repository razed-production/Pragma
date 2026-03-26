#include "Pragma/Core/SceneDocument.h"

#include "Pragma/Assets/AssetManager.h"
#include "Pragma/Core/Assert.h"
#include "Pragma/Core/EngineInput.h"
#include "Pragma/Core/PrefabSerializer.h"
#include "Pragma/Platform/InputState.h"
#include "Pragma/Physics/BoxColliderComponent.h"
#include "Pragma/Physics/RigidBodyComponent.h"
#include "Pragma/Renderer/CameraComponent.h"
#include "Pragma/Renderer/CameraControllerComponent.h"
#include "Pragma/Renderer/LightComponent.h"
#include "Pragma/Renderer/ManagedScriptComponent.h"
#include "Pragma/Renderer/MeshRendererComponent.h"
#include "Pragma/Renderer/NativeScriptComponent.h"
#include "Pragma/Renderer/NativeScriptRegistry.h"
#include "Pragma/Scripting/ManagedScriptHost.h"

#include <algorithm>
#include <cstring>
#include <unordered_map>

namespace Pragma::Core
{
namespace
{
bool RemoveBehaviourComponent(Pragma::Renderer::SceneObject& object) noexcept
{
    bool removed = object.RemoveComponent<Pragma::Renderer::BehaviourComponent>();
    removed = object.RemoveComponent<Pragma::Renderer::ManagedScriptComponent>() || removed;
    return removed;
}

bool SceneContainsDirectionalLight(const SerializedScene& scene, const Pragma::Renderer::EntityId ignoredId = Pragma::Renderer::InvalidEntityId)
{
    for (const SerializedSceneObject& object : scene.Objects)
    {
        if (object.Id != ignoredId && object.Light.has_value())
        {
            return true;
        }
    }

    return false;
}

bool PrefabContainsDirectionalLight(const SerializedPrefab& prefab)
{
    return std::any_of(
        prefab.Objects.begin(),
        prefab.Objects.end(),
        [](const SerializedSceneObject& object)
        {
            return object.Light.has_value();
        });
}

Pragma::Renderer::EntityId FindPrefabRootId(const SerializedPrefab& prefab) noexcept
{
    for (const SerializedSceneObject& object : prefab.Objects)
    {
        if (object.ParentId == Pragma::Renderer::InvalidEntityId)
        {
            return object.Id;
        }
    }

    return Pragma::Renderer::InvalidEntityId;
}
}

SceneDocument::SceneDocument(
    Pragma::RHI::IDevice& device,
    Pragma::Assets::AssetManager& assets,
    Pragma::Renderer::NativeScriptRegistry& scriptRegistry,
    Pragma::Scripting::ManagedScriptHost& managedScriptHost)
    : m_device(device)
    , m_assets(assets)
    , m_scriptRegistry(scriptRegistry)
    , m_managedScriptHost(managedScriptHost)
    , m_runtimeBuilder(assets, scriptRegistry, managedScriptHost)
{
}

void SceneDocument::LoadFromAsset(const Pragma::Assets::AssetId& sceneAssetId)
{
    m_path = m_assets.ResolvePath(sceneAssetId);
    m_serializedScene = LoadSceneFromFile(m_path);
    m_savedSceneSnapshot = m_serializedScene;
    m_history.Reset();
    m_lastHistoryAction.clear();
    BuildRuntimeScene();
    m_loaded = true;
    m_dirty = false;
}

void SceneDocument::Reload()
{
    PRAGMA_ASSERT(m_loaded, "SceneDocument::Reload requires a loaded document.");
    m_serializedScene = LoadSceneFromFile(m_path);
    m_savedSceneSnapshot = m_serializedScene;
    m_history.Reset();
    m_lastHistoryAction = "Reload Scene";
    BuildRuntimeScene();
    m_dirty = false;
}

void SceneDocument::Save()
{
    PRAGMA_ASSERT(m_loaded, "SceneDocument::Save requires a loaded document.");
    ApplyRuntimeStateToSerializedScene();
    SaveSceneToFile(m_serializedScene, m_path);
    m_savedSceneSnapshot = m_serializedScene;
    m_history.Reset();
    m_lastHistoryAction = "Save Scene";
    m_dirty = false;
    Pragma::Core::Log(Pragma::Core::LogCategory::Scene, Pragma::Core::LogLevel::Info, "Scene saved: " + m_path.string());
}

void SceneDocument::Shutdown()
{
    if (!m_scene.IsInitialized())
    {
        return;
    }

    const EngineTime shutdownTime
    {
        0.0f,
        m_scene.GetElapsedSeconds(),
        static_cast<std::uint64_t>(m_scene.GetFrameIndex())
    };
    const Pragma::Platform::InputState emptyInputState{};
    const EngineInput shutdownInput(emptyInputState);
    m_scene.Shutdown(shutdownTime, shutdownInput);
}

void SceneDocument::MarkDirty() noexcept
{
    if (!m_loaded)
    {
        return;
    }

    UpdateDirtyState();
}

void SceneDocument::CaptureUndoState(const std::string& label)
{
    PRAGMA_ASSERT(m_loaded, "SceneDocument::CaptureUndoState requires a loaded document.");

    const SerializedScene snapshot = CaptureCurrentSceneState();
    m_history.Capture(label, snapshot, SceneStateBridge::AreEqual);
}

bool SceneDocument::Undo()
{
    PRAGMA_ASSERT(m_loaded, "SceneDocument::Undo requires a loaded document.");

    if (!m_history.CanUndo())
    {
        return false;
    }

    const SerializedScene currentSnapshot = CaptureCurrentSceneState();
    const SceneHistory::Entry previousEntry = m_history.PopUndo(currentSnapshot);
    m_lastHistoryAction = "Undo " + previousEntry.Label;
    RestoreSceneState(previousEntry.Snapshot);
    return true;
}

bool SceneDocument::Redo()
{
    PRAGMA_ASSERT(m_loaded, "SceneDocument::Redo requires a loaded document.");

    if (!m_history.CanRedo())
    {
        return false;
    }

    const SerializedScene currentSnapshot = CaptureCurrentSceneState();
    const SceneHistory::Entry nextEntry = m_history.PopRedo(currentSnapshot);
    m_lastHistoryAction = "Redo " + nextEntry.Label;
    RestoreSceneState(nextEntry.Snapshot);
    return true;
}

Pragma::Renderer::EntityId SceneDocument::CreateObject(
    const std::string& name,
    const SceneObjectTemplate objectTemplate,
    const Pragma::Renderer::EntityId parentId)
{
    PRAGMA_ASSERT(m_loaded, "SceneDocument::CreateObject requires a loaded document.");
    if (name.empty())
    {
        return Pragma::Renderer::InvalidEntityId;
    }

    if (parentId != Pragma::Renderer::InvalidEntityId && FindSerializedObject(parentId) == nullptr)
    {
        return Pragma::Renderer::InvalidEntityId;
    }

    if (objectTemplate == SceneObjectTemplate::DirectionalLight && SceneContainsDirectionalLight(m_serializedScene))
    {
        return Pragma::Renderer::InvalidEntityId;
    }

    CaptureUndoState("Create Object");

    SerializedSceneObject serializedObject;
    serializedObject.Id = GenerateNextEntityId();
    serializedObject.ParentId = parentId;
    serializedObject.Name = name;

    switch (objectTemplate)
    {
    case SceneObjectTemplate::Empty:
        break;
    case SceneObjectTemplate::Cube:
        serializedObject.MeshRenderer.emplace();
        serializedObject.MeshRenderer->MeshAsset = { "mesh.cube" };
        serializedObject.MeshRenderer->MaterialAsset = { "material.default_checker" };
        break;
    case SceneObjectTemplate::Camera:
        serializedObject.Transform.Position = { 0.0f, 2.0f, -6.0f };
        serializedObject.Camera.emplace();
        serializedObject.CameraController.emplace();
        serializedObject.IsActiveCamera = m_scene.GetActiveCameraEntityId() == Pragma::Renderer::InvalidEntityId;
        break;
    case SceneObjectTemplate::DirectionalLight:
        serializedObject.Light.emplace();
        break;
    case SceneObjectTemplate::PhysicsCube:
        serializedObject.Transform.Position = { 0.0f, 5.0f, 0.0f };
        serializedObject.MeshRenderer.emplace();
        serializedObject.MeshRenderer->MeshAsset = { "mesh.cube" };
        serializedObject.MeshRenderer->MaterialAsset = { "material.physics_a" };
        serializedObject.RigidBody.emplace();
        serializedObject.BoxCollider.emplace();
        break;
    }

    m_serializedScene.Objects.push_back(serializedObject);
    BuildRuntimeScene();
    UpdateDirtyState();
    return serializedObject.Id;
}

bool SceneDocument::DeleteObject(const Pragma::Renderer::EntityId id)
{
    PRAGMA_ASSERT(m_loaded, "SceneDocument::DeleteObject requires a loaded document.");

    const auto serializedIt = std::find_if(
        m_serializedScene.Objects.begin(),
        m_serializedScene.Objects.end(),
        [id](const SerializedSceneObject& object)
        {
            return object.Id == id;
        });

    if (serializedIt == m_serializedScene.Objects.end())
    {
        return false;
    }

    if (serializedIt->IsActiveCamera)
    {
        return false;
    }

    const std::vector<Pragma::Renderer::EntityId> subtree = m_scene.GetSubtree(id);
    if (std::find(subtree.begin(), subtree.end(), m_scene.GetActiveCameraEntityId()) != subtree.end())
    {
        return false;
    }

    CaptureUndoState("Delete Object");

    for (auto it = m_serializedScene.Objects.begin(); it != m_serializedScene.Objects.end();)
    {
        if (std::find(subtree.begin(), subtree.end(), it->Id) != subtree.end())
        {
            it = m_serializedScene.Objects.erase(it);
        }
        else
        {
            ++it;
        }
    }

    for (auto it = subtree.rbegin(); it != subtree.rend(); ++it)
    {
        (void)m_scene.DestroyObject(*it);
    }

    UpdateDirtyState();
    return true;
}

Pragma::Renderer::EntityId SceneDocument::DuplicateObject(const Pragma::Renderer::EntityId id)
{
    PRAGMA_ASSERT(m_loaded, "SceneDocument::DuplicateObject requires a loaded document.");

    const SerializedSceneObject* source = FindSerializedObject(id);
    if (source == nullptr)
    {
        return Pragma::Renderer::InvalidEntityId;
    }

    const std::vector<Pragma::Renderer::EntityId> subtree = m_scene.GetSubtree(id);
    const bool subtreeContainsLight = std::any_of(
        subtree.begin(),
        subtree.end(),
        [this](const Pragma::Renderer::EntityId subtreeId)
        {
            const SerializedSceneObject* object = FindSerializedObject(subtreeId);
            return object != nullptr && object->Light.has_value();
        });

    if (subtreeContainsLight)
    {
        return Pragma::Renderer::InvalidEntityId;
    }

    CaptureUndoState("Duplicate Object");

    std::unordered_map<Pragma::Renderer::EntityId, Pragma::Renderer::EntityId> duplicatedIds;
    duplicatedIds.reserve(subtree.size());

    std::vector<SerializedSceneObject> duplicates;
    duplicates.reserve(subtree.size());

    for (const Pragma::Renderer::EntityId subtreeId : subtree)
    {
        const SerializedSceneObject* serializedObject = FindSerializedObject(subtreeId);
        if (serializedObject == nullptr)
        {
            continue;
        }

        SerializedSceneObject duplicate = *serializedObject;
        duplicate.Id = GenerateNextEntityId();
        duplicate.IsActiveCamera = false;
        if (subtreeId == id)
        {
            duplicate.Name += " Copy";
            duplicate.ParentId = serializedObject->ParentId;
        }

        duplicatedIds.emplace(subtreeId, duplicate.Id);
        duplicates.push_back(std::move(duplicate));
    }

    for (SerializedSceneObject& duplicate : duplicates)
    {
        if (duplicate.ParentId != Pragma::Renderer::InvalidEntityId)
        {
            const auto parentIt = duplicatedIds.find(duplicate.ParentId);
            if (parentIt != duplicatedIds.end())
            {
                duplicate.ParentId = parentIt->second;
            }
        }
    }

    m_serializedScene.Objects.insert(m_serializedScene.Objects.end(), duplicates.begin(), duplicates.end());

    BuildRuntimeScene();
    UpdateDirtyState();
    const auto duplicatedRootIt = duplicatedIds.find(id);
    return duplicatedRootIt != duplicatedIds.end() ? duplicatedRootIt->second : Pragma::Renderer::InvalidEntityId;
}

bool SceneDocument::RenameObject(const Pragma::Renderer::EntityId id, const std::string& name)
{
    PRAGMA_ASSERT(m_loaded, "SceneDocument::RenameObject requires a loaded document.");

    if (name.empty())
    {
        return false;
    }

    SerializedSceneObject* serializedObject = FindSerializedObject(id);
    Pragma::Renderer::SceneObject* runtimeObject = m_scene.FindObject(id);
    if (serializedObject == nullptr || runtimeObject == nullptr)
    {
        return false;
    }

    CaptureUndoState("Rename Object");

    serializedObject->Name = name;
    runtimeObject->Name = name;
    UpdateDirtyState();
    return true;
}

bool SceneDocument::SetParent(const Pragma::Renderer::EntityId childId, const Pragma::Renderer::EntityId parentId)
{
    PRAGMA_ASSERT(m_loaded, "SceneDocument::SetParent requires a loaded document.");

    if (childId == Pragma::Renderer::InvalidEntityId || parentId == childId)
    {
        return false;
    }

    SerializedSceneObject* serializedObject = FindSerializedObject(childId);
    if (serializedObject == nullptr)
    {
        return false;
    }

    if (parentId != Pragma::Renderer::InvalidEntityId && FindSerializedObject(parentId) == nullptr)
    {
        return false;
    }

    if (parentId != Pragma::Renderer::InvalidEntityId && m_scene.IsDescendant(parentId, childId))
    {
        return false;
    }

    CaptureUndoState("Reparent Object");

    if (!m_scene.SetParent(childId, parentId))
    {
        return false;
    }

    serializedObject->ParentId = parentId;
    UpdateDirtyState();
    return true;
}

bool SceneDocument::AddComponent(const Pragma::Renderer::EntityId id, const Pragma::Renderer::ComponentType componentType)
{
    PRAGMA_ASSERT(m_loaded, "SceneDocument::AddComponent requires a loaded document.");

    SerializedSceneObject* serializedObject = FindSerializedObject(id);
    Pragma::Renderer::SceneObject* runtimeObject = m_scene.FindObject(id);
    if (serializedObject == nullptr || runtimeObject == nullptr)
    {
        return false;
    }

    CaptureUndoState("Add Component");

    switch (componentType)
    {
    case Pragma::Renderer::ComponentType::Camera:
        if (serializedObject->Camera.has_value())
        {
            return false;
        }
        serializedObject->Camera.emplace();
        runtimeObject->AddComponent(std::make_shared<Pragma::Renderer::CameraComponent>(*serializedObject->Camera));
        break;
    case Pragma::Renderer::ComponentType::CameraController:
        if (!serializedObject->Camera.has_value() || serializedObject->CameraController.has_value())
        {
            return false;
        }
        serializedObject->CameraController.emplace();
        runtimeObject->AddComponent(std::make_shared<Pragma::Renderer::CameraControllerComponent>(*serializedObject->CameraController));
        break;
    case Pragma::Renderer::ComponentType::Light:
        if (serializedObject->Light.has_value() || SceneContainsDirectionalLight(m_serializedScene, id))
        {
            return false;
        }
        serializedObject->Light.emplace();
        runtimeObject->AddComponent(std::make_shared<Pragma::Renderer::LightComponent>(*serializedObject->Light));
        break;
    case Pragma::Renderer::ComponentType::MeshRenderer:
        if (serializedObject->MeshRenderer.has_value())
        {
            return false;
        }
        serializedObject->MeshRenderer.emplace();
        serializedObject->MeshRenderer->MeshAsset = { "mesh.cube" };
        serializedObject->MeshRenderer->MaterialAsset = { "material.default_checker" };
        BuildRuntimeScene();
        UpdateDirtyState();
        return true;
    case Pragma::Renderer::ComponentType::RigidBody:
        if (serializedObject->RigidBody.has_value())
        {
            return false;
        }
        if (!serializedObject->BoxCollider.has_value())
        {
            serializedObject->BoxCollider.emplace();
            runtimeObject->AddComponent(std::make_shared<Pragma::Renderer::BoxColliderComponent>(*serializedObject->BoxCollider));
        }
        serializedObject->RigidBody.emplace();
        runtimeObject->AddComponent(std::make_shared<Pragma::Renderer::RigidBodyComponent>(*serializedObject->RigidBody));
        break;
    case Pragma::Renderer::ComponentType::BoxCollider:
        if (serializedObject->BoxCollider.has_value())
        {
            return false;
        }
        if (!serializedObject->RigidBody.has_value())
        {
            serializedObject->RigidBody.emplace();
            runtimeObject->AddComponent(std::make_shared<Pragma::Renderer::RigidBodyComponent>(*serializedObject->RigidBody));
        }
        serializedObject->BoxCollider.emplace();
        runtimeObject->AddComponent(std::make_shared<Pragma::Renderer::BoxColliderComponent>(*serializedObject->BoxCollider));
        break;
    case Pragma::Renderer::ComponentType::Transform:
    case Pragma::Renderer::ComponentType::Behaviour:
    default:
        return false;
    }

    UpdateDirtyState();
    return true;
}

bool SceneDocument::RemoveComponent(const Pragma::Renderer::EntityId id, const Pragma::Renderer::ComponentType componentType)
{
    PRAGMA_ASSERT(m_loaded, "SceneDocument::RemoveComponent requires a loaded document.");

    SerializedSceneObject* serializedObject = FindSerializedObject(id);
    Pragma::Renderer::SceneObject* runtimeObject = m_scene.FindObject(id);
    if (serializedObject == nullptr || runtimeObject == nullptr)
    {
        return false;
    }

    CaptureUndoState("Remove Component");

    switch (componentType)
    {
    case Pragma::Renderer::ComponentType::Camera:
        if (!serializedObject->Camera.has_value() || m_scene.GetActiveCameraEntityId() == id)
        {
            return false;
        }
        serializedObject->Camera.reset();
        serializedObject->CameraController.reset();
        runtimeObject->RemoveComponent<Pragma::Renderer::CameraControllerComponent>();
        if (!runtimeObject->RemoveComponent<Pragma::Renderer::CameraComponent>())
        {
            return false;
        }
        UpdateDirtyState();
        return true;
    case Pragma::Renderer::ComponentType::CameraController:
        if (!serializedObject->CameraController.has_value())
        {
            return false;
        }
        serializedObject->CameraController.reset();
        if (!runtimeObject->RemoveComponent<Pragma::Renderer::CameraControllerComponent>())
        {
            return false;
        }
        UpdateDirtyState();
        return true;
    case Pragma::Renderer::ComponentType::Light:
        if (!serializedObject->Light.has_value())
        {
            return false;
        }
        serializedObject->Light.reset();
        if (!runtimeObject->RemoveComponent<Pragma::Renderer::LightComponent>())
        {
            return false;
        }
        UpdateDirtyState();
        return true;
    case Pragma::Renderer::ComponentType::MeshRenderer:
        if (!serializedObject->MeshRenderer.has_value())
        {
            return false;
        }
        serializedObject->MeshRenderer.reset();
        if (!runtimeObject->RemoveComponent<Pragma::Renderer::MeshRendererComponent>())
        {
            return false;
        }
        UpdateDirtyState();
        return true;
    case Pragma::Renderer::ComponentType::Transform:
        return false;
    case Pragma::Renderer::ComponentType::Behaviour:
        if (serializedObject->ScriptName.empty() && serializedObject->ManagedScriptProjectAsset.empty())
        {
            return false;
        }
        serializedObject->ScriptName.clear();
        serializedObject->ManagedScriptProjectAsset = {};
        serializedObject->ManagedScriptTypeName.clear();
        if (!RemoveBehaviourComponent(*runtimeObject))
        {
            return false;
        }
        UpdateDirtyState();
        return true;
    case Pragma::Renderer::ComponentType::RigidBody:
    case Pragma::Renderer::ComponentType::BoxCollider:
        if (!serializedObject->RigidBody.has_value() || !serializedObject->BoxCollider.has_value())
        {
            return false;
        }
        serializedObject->RigidBody.reset();
        serializedObject->BoxCollider.reset();
        runtimeObject->RemoveComponent<Pragma::Renderer::RigidBodyComponent>();
        runtimeObject->RemoveComponent<Pragma::Renderer::BoxColliderComponent>();
        UpdateDirtyState();
        return true;
    default:
        return false;
    }
}

bool SceneDocument::SetMaterialAsset(const Pragma::Renderer::EntityId id, const Pragma::Assets::AssetId& materialAssetId)
{
    PRAGMA_ASSERT(m_loaded, "SceneDocument::SetMaterialAsset requires a loaded document.");

    if (materialAssetId.empty())
    {
        return false;
    }

    SerializedSceneObject* serializedObject = FindSerializedObject(id);
    Pragma::Renderer::SceneObject* runtimeObject = m_scene.FindObject(id);
    if (serializedObject == nullptr || runtimeObject == nullptr || !serializedObject->MeshRenderer.has_value())
    {
        return false;
    }

    Pragma::Renderer::MeshRendererComponent* meshRenderer = runtimeObject->GetMeshRenderer();
    if (meshRenderer == nullptr)
    {
        return false;
    }

    CaptureUndoState("Assign Material");

    serializedObject->MeshRenderer->MaterialAsset = materialAssetId;
    meshRenderer->MaterialAssetId = materialAssetId;
    meshRenderer->Material = m_assets.LoadMaterial(materialAssetId);
    UpdateDirtyState();
    return true;
}

Pragma::Renderer::EntityId SceneDocument::InstantiatePrefab(
    const Pragma::Assets::AssetId& prefabAssetId,
    const Pragma::Renderer::EntityId parentId)
{
    PRAGMA_ASSERT(m_loaded, "SceneDocument::InstantiatePrefab requires a loaded document.");

    if (prefabAssetId.empty())
    {
        return Pragma::Renderer::InvalidEntityId;
    }

    if (parentId != Pragma::Renderer::InvalidEntityId && FindSerializedObject(parentId) == nullptr)
    {
        return Pragma::Renderer::InvalidEntityId;
    }

    const SerializedPrefab prefab = LoadPrefabFromFile(m_assets.ResolvePath(prefabAssetId));
    if (prefab.Objects.empty())
    {
        return Pragma::Renderer::InvalidEntityId;
    }

    if (PrefabContainsDirectionalLight(prefab) && SceneContainsDirectionalLight(m_serializedScene))
    {
        return Pragma::Renderer::InvalidEntityId;
    }

    CaptureUndoState("Instantiate Prefab");

    Pragma::Renderer::EntityId nextId = GenerateNextEntityId();
    std::unordered_map<Pragma::Renderer::EntityId, Pragma::Renderer::EntityId> remappedIds;
    remappedIds.reserve(prefab.Objects.size());

    std::vector<SerializedSceneObject> instantiatedObjects;
    instantiatedObjects.reserve(prefab.Objects.size());

    Pragma::Renderer::EntityId rootPrefabId = Pragma::Renderer::InvalidEntityId;
    for (const SerializedSceneObject& prefabObject : prefab.Objects)
    {
        SerializedSceneObject instance = prefabObject;
        if (prefabObject.ParentId == Pragma::Renderer::InvalidEntityId)
        {
            rootPrefabId = prefabObject.Id;
            instance.ParentId = parentId;
            instance.PrefabAssetId = prefabAssetId;
        }
        else
        {
            instance.PrefabAssetId = {};
        }

        instance.Id = nextId++;
        instance.IsActiveCamera = false;
        remappedIds.emplace(prefabObject.Id, instance.Id);
        instantiatedObjects.push_back(std::move(instance));
    }

    for (SerializedSceneObject& instance : instantiatedObjects)
    {
        if (instance.ParentId == Pragma::Renderer::InvalidEntityId)
        {
            continue;
        }

        const auto parentIt = remappedIds.find(instance.ParentId);
        if (parentIt != remappedIds.end())
        {
            instance.ParentId = parentIt->second;
        }
    }

    m_serializedScene.Objects.insert(m_serializedScene.Objects.end(), instantiatedObjects.begin(), instantiatedObjects.end());
    BuildRuntimeScene();
    UpdateDirtyState();

    const auto rootIt = remappedIds.find(rootPrefabId);
    return rootIt != remappedIds.end() ? rootIt->second : Pragma::Renderer::InvalidEntityId;
}

bool SceneDocument::SavePrefab(const Pragma::Renderer::EntityId rootId, const Pragma::Assets::AssetId& prefabAssetId)
{
    PRAGMA_ASSERT(m_loaded, "SceneDocument::SavePrefab requires a loaded document.");

    if (prefabAssetId.empty() || prefabAssetId.Value.rfind("prefab.", 0) != 0)
    {
        return false;
    }

    std::string prefabFileStem = prefabAssetId.Value.substr(std::strlen("prefab."));
    if (prefabFileStem.empty())
    {
        return false;
    }

    std::replace(prefabFileStem.begin(), prefabFileStem.end(), '.', '_');
    const std::filesystem::path relativePrefabPath = std::filesystem::path("prefabs") / (prefabFileStem + ".prefab");
    const std::filesystem::path absolutePrefabPath = m_path.parent_path().parent_path() / relativePrefabPath;

    SerializedPrefab prefab;
    if (!TryBuildPrefabSnapshot(rootId, prefab))
    {
        return false;
    }

    std::filesystem::create_directories(absolutePrefabPath.parent_path());
    SavePrefabToFile(prefab, absolutePrefabPath);
    m_assets.RegisterAsset(prefabAssetId, relativePrefabPath);
    Pragma::Core::Log(Pragma::Core::LogCategory::Scene, Pragma::Core::LogLevel::Info, "Prefab saved: " + absolutePrefabPath.string());
    return true;
}

bool SceneDocument::ApplyPrefab(const Pragma::Renderer::EntityId rootId)
{
    PRAGMA_ASSERT(m_loaded, "SceneDocument::ApplyPrefab requires a loaded document.");

    Pragma::Assets::AssetId prefabAssetId;
    SerializedPrefab prefab;
    if (!TryBuildPrefabSnapshot(rootId, prefab, &prefabAssetId) || prefabAssetId.empty())
    {
        return false;
    }

    std::string prefabFileStem = prefabAssetId.Value.substr(std::strlen("prefab."));
    std::replace(prefabFileStem.begin(), prefabFileStem.end(), '.', '_');
    const std::filesystem::path relativePrefabPath = std::filesystem::path("prefabs") / (prefabFileStem + ".prefab");
    const std::filesystem::path absolutePrefabPath = m_path.parent_path().parent_path() / relativePrefabPath;

    std::filesystem::create_directories(absolutePrefabPath.parent_path());
    SavePrefabToFile(prefab, absolutePrefabPath);
    m_assets.RegisterAsset(prefabAssetId, relativePrefabPath);
    Pragma::Core::Log(Pragma::Core::LogCategory::Scene, Pragma::Core::LogLevel::Info, "Prefab applied: " + absolutePrefabPath.string());
    return true;
}

Pragma::Renderer::EntityId SceneDocument::RevertPrefab(const Pragma::Renderer::EntityId rootId)
{
    PRAGMA_ASSERT(m_loaded, "SceneDocument::RevertPrefab requires a loaded document.");

    const SerializedSceneObject* rootObject = FindSerializedObject(rootId);
    if (rootObject == nullptr || rootObject->PrefabAssetId.empty())
    {
        return Pragma::Renderer::InvalidEntityId;
    }

    const SerializedPrefab prefab = LoadPrefabFromFile(m_assets.ResolvePath(rootObject->PrefabAssetId));
    const std::vector<Pragma::Renderer::EntityId> subtree = m_scene.GetSubtree(rootId);
    const bool subtreeContainsActiveCamera =
        std::find(subtree.begin(), subtree.end(), m_scene.GetActiveCameraEntityId()) != subtree.end();
    if (subtreeContainsActiveCamera)
    {
        return Pragma::Renderer::InvalidEntityId;
    }

    if (PrefabContainsDirectionalLight(prefab))
    {
        bool sceneHasOtherDirectionalLight = false;
        for (const SerializedSceneObject& object : m_serializedScene.Objects)
        {
            if (!object.Light.has_value())
            {
                continue;
            }

            if (std::find(subtree.begin(), subtree.end(), object.Id) == subtree.end())
            {
                sceneHasOtherDirectionalLight = true;
                break;
            }
        }

        if (sceneHasOtherDirectionalLight)
        {
            return Pragma::Renderer::InvalidEntityId;
        }
    }

    CaptureUndoState("Revert Prefab");

    const Pragma::Renderer::EntityId rootParentId = rootObject->ParentId;
    const auto eraseIt = std::remove_if(
        m_serializedScene.Objects.begin(),
        m_serializedScene.Objects.end(),
        [&subtree](const SerializedSceneObject& object)
        {
            return std::find(subtree.begin(), subtree.end(), object.Id) != subtree.end();
        });
    m_serializedScene.Objects.erase(eraseIt, m_serializedScene.Objects.end());

    Pragma::Renderer::EntityId nextId = GenerateNextEntityId();
    std::unordered_map<Pragma::Renderer::EntityId, Pragma::Renderer::EntityId> remappedIds;
    remappedIds.reserve(prefab.Objects.size());

    std::vector<SerializedSceneObject> instantiatedObjects;
    instantiatedObjects.reserve(prefab.Objects.size());

    const Pragma::Renderer::EntityId prefabRootId = FindPrefabRootId(prefab);
    for (const SerializedSceneObject& prefabObject : prefab.Objects)
    {
        SerializedSceneObject instance = prefabObject;
        if (prefabObject.ParentId == Pragma::Renderer::InvalidEntityId)
        {
            instance.ParentId = rootParentId;
            instance.PrefabAssetId = rootObject->PrefabAssetId;
        }
        else
        {
            instance.PrefabAssetId = {};
        }

        instance.Id = nextId++;
        instance.IsActiveCamera = false;
        remappedIds.emplace(prefabObject.Id, instance.Id);
        instantiatedObjects.push_back(std::move(instance));
    }

    for (SerializedSceneObject& instance : instantiatedObjects)
    {
        if (instance.ParentId == Pragma::Renderer::InvalidEntityId || instance.ParentId == rootParentId)
        {
            continue;
        }

        const auto parentIt = remappedIds.find(instance.ParentId);
        if (parentIt != remappedIds.end())
        {
            instance.ParentId = parentIt->second;
        }
    }

    m_serializedScene.Objects.insert(m_serializedScene.Objects.end(), instantiatedObjects.begin(), instantiatedObjects.end());
    BuildRuntimeScene();
    UpdateDirtyState();

    const auto rootIt = remappedIds.find(prefabRootId);
    return rootIt != remappedIds.end() ? rootIt->second : Pragma::Renderer::InvalidEntityId;
}

bool SceneDocument::HasPrefabOverrides(const Pragma::Renderer::EntityId rootId) const
{
    PRAGMA_ASSERT(m_loaded, "SceneDocument::HasPrefabOverrides requires a loaded document.");

    Pragma::Assets::AssetId prefabAssetId;
    SerializedPrefab runtimePrefab;
    if (!TryBuildPrefabSnapshot(rootId, runtimePrefab, &prefabAssetId) || prefabAssetId.empty())
    {
        return false;
    }

    const SerializedPrefab sourcePrefab = LoadPrefabFromFile(m_assets.ResolvePath(prefabAssetId));
    if (runtimePrefab.Objects.size() != sourcePrefab.Objects.size())
    {
        return true;
    }

    for (std::size_t index = 0; index < runtimePrefab.Objects.size(); ++index)
    {
        const SerializedSceneObject& lhs = runtimePrefab.Objects[index];
        const SerializedSceneObject& rhs = sourcePrefab.Objects[index];
        if (!SceneStateBridge::AreEqual({ 11, { lhs } }, { 11, { rhs } }))
        {
            return true;
        }
    }

    return false;
}

bool SceneDocument::SetScript(const Pragma::Renderer::EntityId id, const std::string& scriptName)
{
    PRAGMA_ASSERT(m_loaded, "SceneDocument::SetScript requires a loaded document.");

    if (scriptName.empty() || !m_scriptRegistry.IsRegistered(scriptName))
    {
        return false;
    }

    SerializedSceneObject* serializedObject = FindSerializedObject(id);
    Pragma::Renderer::SceneObject* runtimeObject = m_scene.FindObject(id);
    if (serializedObject == nullptr || runtimeObject == nullptr)
    {
        return false;
    }

    if (serializedObject->ScriptName == scriptName)
    {
        return true;
    }

    CaptureUndoState("Assign Script");

    std::shared_ptr<Pragma::Renderer::NativeScriptComponent> script = m_scriptRegistry.Create(scriptName);
    PRAGMA_ASSERT(script != nullptr, "SceneDocument failed to create a registered native script.");

    RemoveBehaviourComponent(*runtimeObject);
    serializedObject->ManagedScriptProjectAsset = {};
    serializedObject->ManagedScriptTypeName.clear();
    runtimeObject->AddComponent(script);
    serializedObject->ScriptName = scriptName;
    UpdateDirtyState();
    return true;
}

bool SceneDocument::SetManagedScript(
    const Pragma::Renderer::EntityId id,
    const Pragma::Assets::AssetId& projectAssetId,
    const std::string_view typeName)
{
    PRAGMA_ASSERT(m_loaded, "SceneDocument::SetManagedScript requires a loaded document.");

    if (projectAssetId.empty() || typeName.empty())
    {
        return false;
    }

    SerializedSceneObject* serializedObject = FindSerializedObject(id);
    Pragma::Renderer::SceneObject* runtimeObject = m_scene.FindObject(id);
    if (serializedObject == nullptr || runtimeObject == nullptr)
    {
        return false;
    }

    if (serializedObject->ManagedScriptProjectAsset.Value == projectAssetId.Value &&
        serializedObject->ManagedScriptTypeName == typeName)
    {
        return true;
    }

    CaptureUndoState("Assign Managed Script");

    RemoveBehaviourComponent(*runtimeObject);
    serializedObject->ScriptName.clear();
    serializedObject->ManagedScriptProjectAsset = projectAssetId;
    serializedObject->ManagedScriptTypeName = std::string(typeName);
    runtimeObject->AddComponent(std::make_shared<Pragma::Renderer::ManagedScriptComponent>(
        serializedObject->ManagedScriptProjectAsset,
        serializedObject->ManagedScriptTypeName));
    UpdateDirtyState();
    return true;
}

bool SceneDocument::ClearScript(const Pragma::Renderer::EntityId id)
{
    PRAGMA_ASSERT(m_loaded, "SceneDocument::ClearScript requires a loaded document.");

    SerializedSceneObject* serializedObject = FindSerializedObject(id);
    Pragma::Renderer::SceneObject* runtimeObject = m_scene.FindObject(id);
    if (serializedObject == nullptr || runtimeObject == nullptr ||
        (serializedObject->ScriptName.empty() && serializedObject->ManagedScriptProjectAsset.empty()))
    {
        return false;
    }

    CaptureUndoState("Clear Script");

    serializedObject->ScriptName.clear();
    serializedObject->ManagedScriptProjectAsset = {};
    serializedObject->ManagedScriptTypeName.clear();
    if (!RemoveBehaviourComponent(*runtimeObject))
    {
        return false;
    }

    UpdateDirtyState();
    return true;
}

bool SceneDocument::IsDirty() const noexcept
{
    return m_dirty;
}

bool SceneDocument::IsLoaded() const noexcept
{
    return m_loaded;
}

bool SceneDocument::CanUndo() const noexcept
{
    return m_history.CanUndo();
}

bool SceneDocument::CanRedo() const noexcept
{
    return m_history.CanRedo();
}

const std::string& SceneDocument::GetUndoLabel() const noexcept
{
    return m_history.PeekUndoLabel();
}

const std::string& SceneDocument::GetRedoLabel() const noexcept
{
    return m_history.PeekRedoLabel();
}

const std::string& SceneDocument::GetLastHistoryAction() const noexcept
{
    return m_lastHistoryAction;
}

const std::filesystem::path& SceneDocument::GetPath() const noexcept
{
    return m_path;
}

const Pragma::Renderer::Scene& SceneDocument::GetScene() const noexcept
{
    return m_scene;
}

Pragma::Renderer::Scene& SceneDocument::GetScene() noexcept
{
    return m_scene;
}

void SceneDocument::BuildRuntimeScene()
{
    if (m_scene.IsInitialized())
    {
        Shutdown();
    }

    m_runtimeBuilder.Build(m_scene, m_serializedScene);
    const SerializedScene rebuiltSceneSnapshot = CaptureCurrentSceneState();
    const std::string validationError = SceneStateBridge::DescribeDifference(m_serializedScene, rebuiltSceneSnapshot);
    PRAGMA_ASSERT(validationError.empty(), validationError);
}

void SceneDocument::ApplyRuntimeStateToSerializedScene()
{
    m_serializedScene = SceneStateBridge::Capture(m_scene, m_serializedScene);
}

SerializedScene SceneDocument::CaptureCurrentSceneState() const
{
    return SceneStateBridge::Capture(m_scene, m_serializedScene);
}

void SceneDocument::RestoreSceneState(const SerializedScene& sceneState)
{
    m_serializedScene = sceneState;
    BuildRuntimeScene();
    UpdateDirtyState();
}

void SceneDocument::UpdateDirtyState()
{
    if (!m_loaded)
    {
        m_dirty = false;
        return;
    }

    m_dirty = !SceneStateBridge::AreEqual(CaptureCurrentSceneState(), m_savedSceneSnapshot);
}

SerializedSceneObject* SceneDocument::FindSerializedObject(const Pragma::Renderer::EntityId id) noexcept
{
    for (SerializedSceneObject& object : m_serializedScene.Objects)
    {
        if (object.Id == id)
        {
            return &object;
        }
    }

    return nullptr;
}

const SerializedSceneObject* SceneDocument::FindSerializedObject(const Pragma::Renderer::EntityId id) const noexcept
{
    for (const SerializedSceneObject& object : m_serializedScene.Objects)
    {
        if (object.Id == id)
        {
            return &object;
        }
    }

    return nullptr;
}

Pragma::Renderer::EntityId SceneDocument::GenerateNextEntityId() const noexcept
{
    Pragma::Renderer::EntityId nextId = 1;
    for (const SerializedSceneObject& object : m_serializedScene.Objects)
    {
        nextId = std::max(nextId, object.Id + 1);
    }

    return nextId;
}

bool SceneDocument::TryBuildPrefabSnapshot(
    const Pragma::Renderer::EntityId rootId,
    SerializedPrefab& prefabSnapshot,
    Pragma::Assets::AssetId* sourcePrefabAsset) const
{
    const SerializedSceneObject* rootObject = FindSerializedObject(rootId);
    if (rootObject == nullptr)
    {
        return false;
    }

    if (sourcePrefabAsset != nullptr)
    {
        *sourcePrefabAsset = rootObject->PrefabAssetId;
    }

    prefabSnapshot = {};
    prefabSnapshot.Version = 1;

    const std::vector<Pragma::Renderer::EntityId> subtree = m_scene.GetSubtree(rootId);
    std::unordered_map<Pragma::Renderer::EntityId, Pragma::Renderer::EntityId> prefabIds;
    prefabIds.reserve(subtree.size());

    Pragma::Renderer::EntityId nextPrefabId = 1;
    for (const Pragma::Renderer::EntityId subtreeId : subtree)
    {
        prefabIds.emplace(subtreeId, nextPrefabId++);
    }

    prefabSnapshot.Objects.reserve(subtree.size());
    for (const Pragma::Renderer::EntityId subtreeId : subtree)
    {
        const SerializedSceneObject* serializedObject = FindSerializedObject(subtreeId);
        if (serializedObject == nullptr)
        {
            continue;
        }

        SerializedSceneObject prefabObject = *serializedObject;
        prefabObject.PrefabAssetId = {};
        prefabObject.Id = prefabIds.at(subtreeId);
        prefabObject.IsActiveCamera = false;
        if (subtreeId == rootId)
        {
            prefabObject.ParentId = Pragma::Renderer::InvalidEntityId;
        }
        else
        {
            const auto parentIt = prefabIds.find(serializedObject->ParentId);
            prefabObject.ParentId = parentIt != prefabIds.end() ? parentIt->second : Pragma::Renderer::InvalidEntityId;
        }

        prefabSnapshot.Objects.push_back(std::move(prefabObject));
    }

    return !prefabSnapshot.Objects.empty();
}
}
