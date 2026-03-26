#pragma once

#include "Pragma/Assets/AssetId.h"
#include "Pragma/Core/SceneHistory.h"
#include "Pragma/Core/SceneObjectTemplate.h"
#include "Pragma/Core/PrefabSerializer.h"
#include "Pragma/Core/SceneRuntimeBuilder.h"
#include "Pragma/Core/SceneSerializer.h"
#include "Pragma/Core/SceneStateBridge.h"
#include "Pragma/Renderer/Component.h"
#include "Pragma/Renderer/Entity.h"
#include "Pragma/Renderer/Scene.h"

#include <filesystem>
#include <memory>
namespace Pragma::Assets
{
class AssetManager;
}

namespace Pragma::RHI
{
class IDevice;
}

namespace Pragma::Renderer
{
class NativeScriptRegistry;
struct Material;
}

namespace Pragma::Scripting
{
class ManagedScriptHost;
}

namespace Pragma::Core
{
class SceneDocument
{
public:
    SceneDocument(
        Pragma::RHI::IDevice& device,
        Pragma::Assets::AssetManager& assets,
        Pragma::Renderer::NativeScriptRegistry& scriptRegistry,
        Pragma::Scripting::ManagedScriptHost& managedScriptHost);

    void LoadFromAsset(const Pragma::Assets::AssetId& sceneAssetId);
    void Reload();
    void Save();
    void Shutdown();
    void MarkDirty() noexcept;
    void CaptureUndoState(const std::string& label = "Edit");
    [[nodiscard]] bool Undo();
    [[nodiscard]] bool Redo();
    [[nodiscard]] Pragma::Renderer::EntityId CreateObject(
        const std::string& name,
        SceneObjectTemplate objectTemplate = SceneObjectTemplate::Empty,
        Pragma::Renderer::EntityId parentId = Pragma::Renderer::InvalidEntityId);
    [[nodiscard]] bool DeleteObject(Pragma::Renderer::EntityId id);
    [[nodiscard]] Pragma::Renderer::EntityId DuplicateObject(Pragma::Renderer::EntityId id);
    [[nodiscard]] bool RenameObject(Pragma::Renderer::EntityId id, const std::string& name);
    [[nodiscard]] bool SetParent(Pragma::Renderer::EntityId childId, Pragma::Renderer::EntityId parentId);
    [[nodiscard]] bool AddComponent(Pragma::Renderer::EntityId id, Pragma::Renderer::ComponentType componentType);
    [[nodiscard]] bool RemoveComponent(Pragma::Renderer::EntityId id, Pragma::Renderer::ComponentType componentType);
    [[nodiscard]] bool SetMaterialAsset(Pragma::Renderer::EntityId id, const Pragma::Assets::AssetId& materialAssetId);
    [[nodiscard]] Pragma::Renderer::EntityId InstantiatePrefab(
        const Pragma::Assets::AssetId& prefabAssetId,
        Pragma::Renderer::EntityId parentId = Pragma::Renderer::InvalidEntityId);
    [[nodiscard]] bool SavePrefab(
        Pragma::Renderer::EntityId rootId,
        const Pragma::Assets::AssetId& prefabAssetId);
    [[nodiscard]] bool ApplyPrefab(Pragma::Renderer::EntityId rootId);
    [[nodiscard]] Pragma::Renderer::EntityId RevertPrefab(Pragma::Renderer::EntityId rootId);
    [[nodiscard]] bool HasPrefabOverrides(Pragma::Renderer::EntityId rootId) const;
    [[nodiscard]] bool SetScript(Pragma::Renderer::EntityId id, const std::string& scriptName);
    [[nodiscard]] bool SetManagedScript(
        Pragma::Renderer::EntityId id,
        const Pragma::Assets::AssetId& projectAssetId,
        std::string_view typeName);
    [[nodiscard]] bool ClearScript(Pragma::Renderer::EntityId id);

    [[nodiscard]] bool IsDirty() const noexcept;
    [[nodiscard]] bool IsLoaded() const noexcept;
    [[nodiscard]] bool CanUndo() const noexcept;
    [[nodiscard]] bool CanRedo() const noexcept;
    [[nodiscard]] const std::string& GetUndoLabel() const noexcept;
    [[nodiscard]] const std::string& GetRedoLabel() const noexcept;
    [[nodiscard]] const std::string& GetLastHistoryAction() const noexcept;
    [[nodiscard]] const std::filesystem::path& GetPath() const noexcept;
    [[nodiscard]] const Pragma::Renderer::Scene& GetScene() const noexcept;
    [[nodiscard]] Pragma::Renderer::Scene& GetScene() noexcept;

private:
    void BuildRuntimeScene();
    void ApplyRuntimeStateToSerializedScene();
    [[nodiscard]] SerializedScene CaptureCurrentSceneState() const;
    void RestoreSceneState(const SerializedScene& sceneState);
    void UpdateDirtyState();
    [[nodiscard]] SerializedSceneObject* FindSerializedObject(Pragma::Renderer::EntityId id) noexcept;
    [[nodiscard]] const SerializedSceneObject* FindSerializedObject(Pragma::Renderer::EntityId id) const noexcept;
    [[nodiscard]] Pragma::Renderer::EntityId GenerateNextEntityId() const noexcept;
    [[nodiscard]] bool TryBuildPrefabSnapshot(
        Pragma::Renderer::EntityId rootId,
        SerializedPrefab& prefabSnapshot,
        Pragma::Assets::AssetId* sourcePrefabAsset = nullptr) const;

private:
    Pragma::RHI::IDevice& m_device;
    Pragma::Assets::AssetManager& m_assets;
    Pragma::Renderer::NativeScriptRegistry& m_scriptRegistry;
    Pragma::Scripting::ManagedScriptHost& m_managedScriptHost;
    SceneRuntimeBuilder m_runtimeBuilder;
    Pragma::Renderer::Scene m_scene;
    SerializedScene m_serializedScene;
    SerializedScene m_savedSceneSnapshot;
    SceneHistory m_history;
    std::filesystem::path m_path;
    std::string m_lastHistoryAction;
    bool m_loaded = false;
    bool m_dirty = false;
};
}
