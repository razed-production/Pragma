#pragma once

#include "Pragma/Core/SceneObjectTemplate.h"
#include "Pragma/Renderer/NativeScriptRegistry.h"
#include "Pragma/Renderer/Scene.h"
#include "Pragma/Scripting/ManagedScriptHost.h"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace Pragma::Assets
{
class AssetManager;
struct MaterialAssetData;
}

namespace Pragma::RHI
{
class IDevice;
}

namespace Pragma::Core
{
class EngineInput;
struct EngineTime;
class SceneDocument;

struct ManagedBuildStatus
{
    bool HasRun = false;
    bool Succeeded = false;
    int ExitCode = -1;
    std::filesystem::path ScriptPath;
    std::filesystem::path StandardOutputPath;
    std::filesystem::path StandardErrorPath;
    std::string Summary;
};

class DemoScene
{
public:
    DemoScene(Pragma::RHI::IDevice& device, Pragma::Assets::AssetManager& assets, Pragma::Scripting::ManagedScriptHost& managedScriptHost);
    ~DemoScene();

    void Initialize();
    void Update(const EngineTime& time, const EngineInput& input);
    [[nodiscard]] bool LoadSceneAsset(const Pragma::Assets::AssetId& sceneAssetId);
    void SaveDocument();
    void ReloadDocument();
    void ReloadManagedScripting();
    [[nodiscard]] bool BuildManagedScripts();
    void MarkDocumentDirty() noexcept;
    void CaptureUndoState(const std::string& label = "Edit");
    [[nodiscard]] bool UndoDocument();
    [[nodiscard]] bool RedoDocument();
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
    [[nodiscard]] bool SetMeshAsset(
        Pragma::Renderer::EntityId id,
        Pragma::Renderer::MeshLodSlot slot,
        const Pragma::Assets::AssetId& meshAssetId);
    [[nodiscard]] Pragma::Renderer::EntityId InstantiatePrefab(
        const Pragma::Assets::AssetId& prefabAssetId,
        Pragma::Renderer::EntityId parentId = Pragma::Renderer::InvalidEntityId);
    [[nodiscard]] bool SavePrefab(
        Pragma::Renderer::EntityId rootId,
        const Pragma::Assets::AssetId& prefabAssetId);
    [[nodiscard]] bool ApplyPrefab(Pragma::Renderer::EntityId rootId);
    [[nodiscard]] Pragma::Renderer::EntityId RevertPrefab(Pragma::Renderer::EntityId rootId);
    [[nodiscard]] bool HasPrefabOverrides(Pragma::Renderer::EntityId rootId) const;
    [[nodiscard]] Pragma::Assets::MaterialAssetData LoadMaterialAssetData(const Pragma::Assets::AssetId& materialAssetId) const;
    [[nodiscard]] bool SaveMaterialAssetData(const Pragma::Assets::AssetId& materialAssetId, const Pragma::Assets::MaterialAssetData& materialAssetData);
    [[nodiscard]] bool SetScript(Pragma::Renderer::EntityId id, const std::string& scriptName);
    [[nodiscard]] bool SetManagedScript(
        Pragma::Renderer::EntityId id,
        const Pragma::Assets::AssetId& projectAssetId,
        std::string_view typeName);
    [[nodiscard]] bool ClearScript(Pragma::Renderer::EntityId id);
    [[nodiscard]] bool IsDocumentDirty() const noexcept;
    [[nodiscard]] bool CanUndoDocument() const noexcept;
    [[nodiscard]] bool CanRedoDocument() const noexcept;
    [[nodiscard]] const std::string& GetUndoLabel() const noexcept;
    [[nodiscard]] const std::string& GetRedoLabel() const noexcept;
    [[nodiscard]] const std::string& GetLastHistoryAction() const noexcept;
    [[nodiscard]] const std::filesystem::path& GetDocumentPath() const noexcept;
    [[nodiscard]] std::vector<Pragma::Renderer::NativeScriptMetadata> GetAvailableScripts() const;
    [[nodiscard]] std::vector<Pragma::Scripting::ManagedScriptTypeMetadata> GetAvailableManagedScripts() const;
    [[nodiscard]] const std::vector<Pragma::Scripting::ManagedScriptProject>& GetManagedScriptProjects() const noexcept;
    [[nodiscard]] const ManagedBuildStatus& GetManagedBuildStatus() const noexcept;
    [[nodiscard]] std::vector<std::string> GetAvailableScriptNames() const;
    [[nodiscard]] std::vector<std::string> GetAvailableSceneAssetNames() const;
    [[nodiscard]] std::vector<std::string> GetAvailableMeshAssetNames() const;
    [[nodiscard]] std::vector<std::string> GetAvailableMaterialAssetNames() const;
    [[nodiscard]] std::vector<std::string> GetAvailablePrefabAssetNames() const;
    [[nodiscard]] std::vector<std::string> GetAvailableTextureAssetNames() const;
    [[nodiscard]] std::filesystem::path ResolveAssetPath(const Pragma::Assets::AssetId& assetId) const;
    [[nodiscard]] const Pragma::Assets::AssetId& GetCurrentSceneAssetId() const noexcept;
    [[nodiscard]] const Pragma::Renderer::Scene& GetScene() const noexcept;
    [[nodiscard]] Pragma::Renderer::Scene& GetScene() noexcept;

private:
    void RegisterScripts();

private:
    Pragma::RHI::IDevice& m_device;
    Pragma::Assets::AssetManager& m_assets;
    Pragma::Scripting::ManagedScriptHost& m_managedScriptHost;
    std::unique_ptr<Pragma::Renderer::NativeScriptRegistry> m_scriptRegistry;
    std::unique_ptr<SceneDocument> m_document;
    ManagedBuildStatus m_managedBuildStatus;
    Pragma::Assets::AssetId m_currentSceneAssetId{ "scene.demo" };
    bool m_initialized = false;
};
}
