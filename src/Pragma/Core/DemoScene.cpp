#include "Pragma/Core/DemoScene.h"

#include "Pragma/Assets/AssetManager.h"
#include "Pragma/Core/Assert.h"
#include "Pragma/Core/EngineInput.h"
#include "Pragma/Core/EngineTime.h"
#include "Pragma/Core/Log.h"
#include "Pragma/Core/SceneDocument.h"
#include "Pragma/Renderer/NativeScriptRegistry.h"
#include "Pragma/Renderer/ScriptableEntity.h"
#include "Pragma/Scripting/ManagedScriptHost.h"

#include <algorithm>

namespace Pragma::Core
{
namespace
{
class SpinScript final : public Pragma::Renderer::ScriptableEntity
{
public:
    explicit SpinScript(const float yawSpeedRadiansPerSecond) noexcept
        : m_yawSpeedRadiansPerSecond(yawSpeedRadiansPerSecond)
    {
    }

    void OnStart() override
    {
        Pragma::Renderer::EntityHandle entity = GetEntity();
        PRAGMA_ASSERT(entity.IsValid(), "SpinBehaviour received an invalid entity on start.");

        Pragma::Core::Log(
            Pragma::Core::LogCategory::Scene,
            Pragma::Core::LogLevel::Info,
            "Script started for object '" + std::string(GetName()) + "' (EntityId=" + std::to_string(entity.GetId()) + ").");
    }

    void OnUpdate() override
    {
        Pragma::Renderer::EntityHandle entity = GetEntity();
        PRAGMA_ASSERT(entity.IsValid(), "SpinBehaviour received an invalid entity on update.");
        Pragma::Renderer::Transform* transform = entity.GetTransform();
        PRAGMA_ASSERT(transform != nullptr, "SpinBehaviour failed to resolve transform on update.");
        transform->RotationRadians.Y += GetDeltaSeconds() * m_yawSpeedRadiansPerSecond;
    }

    void OnDestroy() override
    {
        Pragma::Renderer::EntityHandle entity = GetEntity();
        if (!entity.IsValid())
        {
            return;
        }

        Pragma::Core::Log(
            Pragma::Core::LogCategory::Scene,
            Pragma::Core::LogLevel::Info,
            "Script destroyed for object '" + std::string(GetName()) + "' (EntityId=" + std::to_string(entity.GetId()) + ").");
    }

private:
    float m_yawSpeedRadiansPerSecond = 0.0f;
};
}

DemoScene::DemoScene(
    Pragma::RHI::IDevice& device,
    Pragma::Assets::AssetManager& assets,
    Pragma::Scripting::ManagedScriptHost& managedScriptHost)
    : m_device(device)
    , m_assets(assets)
    , m_managedScriptHost(managedScriptHost)
{
}

DemoScene::~DemoScene()
{
    if (m_document != nullptr)
    {
        m_document->Shutdown();
    }
}

void DemoScene::Initialize()
{
    if (m_initialized)
    {
        return;
    }

    RegisterScripts();
    m_document = std::make_unique<SceneDocument>(m_device, m_assets, *m_scriptRegistry, m_managedScriptHost);
    m_document->LoadFromAsset({ "scene.demo" });
    m_initialized = true;
}

void DemoScene::Update(const EngineTime& time, const EngineInput& input)
{
    PRAGMA_ASSERT(m_initialized, "DemoScene must be initialized before Update.");
    m_document->GetScene().Update(time, input);
}

void DemoScene::SaveDocument()
{
    PRAGMA_ASSERT(m_document != nullptr, "DemoScene document must exist before saving.");
    m_document->Save();
}

void DemoScene::ReloadDocument()
{
    PRAGMA_ASSERT(m_document != nullptr, "DemoScene document must exist before reloading.");
    m_document->Reload();
}

void DemoScene::MarkDocumentDirty() noexcept
{
    if (m_document != nullptr)
    {
        m_document->MarkDirty();
    }
}

void DemoScene::CaptureUndoState(const std::string& label)
{
    PRAGMA_ASSERT(m_document != nullptr, "DemoScene document must exist before capturing undo state.");
    m_document->CaptureUndoState(label);
}

bool DemoScene::UndoDocument()
{
    PRAGMA_ASSERT(m_document != nullptr, "DemoScene document must exist before undo.");
    return m_document->Undo();
}

bool DemoScene::RedoDocument()
{
    PRAGMA_ASSERT(m_document != nullptr, "DemoScene document must exist before redo.");
    return m_document->Redo();
}

Pragma::Renderer::EntityId DemoScene::CreateObject(
    const std::string& name,
    const SceneObjectTemplate objectTemplate,
    const Pragma::Renderer::EntityId parentId)
{
    PRAGMA_ASSERT(m_document != nullptr, "DemoScene document must exist before creating an object.");
    return m_document->CreateObject(name, objectTemplate, parentId);
}

bool DemoScene::DeleteObject(const Pragma::Renderer::EntityId id)
{
    PRAGMA_ASSERT(m_document != nullptr, "DemoScene document must exist before deleting an object.");
    return m_document->DeleteObject(id);
}

Pragma::Renderer::EntityId DemoScene::DuplicateObject(const Pragma::Renderer::EntityId id)
{
    PRAGMA_ASSERT(m_document != nullptr, "DemoScene document must exist before duplicating an object.");
    return m_document->DuplicateObject(id);
}

bool DemoScene::RenameObject(const Pragma::Renderer::EntityId id, const std::string& name)
{
    PRAGMA_ASSERT(m_document != nullptr, "DemoScene document must exist before renaming an object.");
    return m_document->RenameObject(id, name);
}

bool DemoScene::SetParent(const Pragma::Renderer::EntityId childId, const Pragma::Renderer::EntityId parentId)
{
    PRAGMA_ASSERT(m_document != nullptr, "DemoScene document must exist before parenting an object.");
    return m_document->SetParent(childId, parentId);
}

bool DemoScene::AddComponent(const Pragma::Renderer::EntityId id, const Pragma::Renderer::ComponentType componentType)
{
    PRAGMA_ASSERT(m_document != nullptr, "DemoScene document must exist before adding a component.");
    return m_document->AddComponent(id, componentType);
}

bool DemoScene::RemoveComponent(const Pragma::Renderer::EntityId id, const Pragma::Renderer::ComponentType componentType)
{
    PRAGMA_ASSERT(m_document != nullptr, "DemoScene document must exist before removing a component.");
    return m_document->RemoveComponent(id, componentType);
}

bool DemoScene::SetMaterialAsset(const Pragma::Renderer::EntityId id, const Pragma::Assets::AssetId& materialAssetId)
{
    PRAGMA_ASSERT(m_document != nullptr, "DemoScene document must exist before assigning a material.");
    return m_document->SetMaterialAsset(id, materialAssetId);
}

Pragma::Renderer::EntityId DemoScene::InstantiatePrefab(
    const Pragma::Assets::AssetId& prefabAssetId,
    const Pragma::Renderer::EntityId parentId)
{
    PRAGMA_ASSERT(m_document != nullptr, "DemoScene document must exist before instantiating a prefab.");
    return m_document->InstantiatePrefab(prefabAssetId, parentId);
}

bool DemoScene::SavePrefab(const Pragma::Renderer::EntityId rootId, const Pragma::Assets::AssetId& prefabAssetId)
{
    PRAGMA_ASSERT(m_document != nullptr, "DemoScene document must exist before saving a prefab.");
    return m_document->SavePrefab(rootId, prefabAssetId);
}

bool DemoScene::ApplyPrefab(const Pragma::Renderer::EntityId rootId)
{
    PRAGMA_ASSERT(m_document != nullptr, "DemoScene document must exist before applying a prefab.");
    return m_document->ApplyPrefab(rootId);
}

Pragma::Renderer::EntityId DemoScene::RevertPrefab(const Pragma::Renderer::EntityId rootId)
{
    PRAGMA_ASSERT(m_document != nullptr, "DemoScene document must exist before reverting a prefab.");
    return m_document->RevertPrefab(rootId);
}

bool DemoScene::HasPrefabOverrides(const Pragma::Renderer::EntityId rootId) const
{
    PRAGMA_ASSERT(m_document != nullptr, "DemoScene document must exist before checking prefab overrides.");
    return m_document->HasPrefabOverrides(rootId);
}

Pragma::Assets::MaterialAssetData DemoScene::LoadMaterialAssetData(const Pragma::Assets::AssetId& materialAssetId) const
{
    return m_assets.GetMaterialAssetData(materialAssetId);
}

bool DemoScene::SaveMaterialAssetData(const Pragma::Assets::AssetId& materialAssetId, const Pragma::Assets::MaterialAssetData& materialAssetData)
{
    return m_assets.SaveMaterialAssetData(materialAssetId, materialAssetData);
}

bool DemoScene::SetScript(const Pragma::Renderer::EntityId id, const std::string& scriptName)
{
    PRAGMA_ASSERT(m_document != nullptr, "DemoScene document must exist before assigning a script.");
    return m_document->SetScript(id, scriptName);
}

bool DemoScene::SetManagedScript(
    const Pragma::Renderer::EntityId id,
    const Pragma::Assets::AssetId& projectAssetId,
    const std::string_view typeName)
{
    PRAGMA_ASSERT(m_document != nullptr, "DemoScene document must exist before assigning a managed script.");
    return m_document->SetManagedScript(id, projectAssetId, typeName);
}

bool DemoScene::ClearScript(const Pragma::Renderer::EntityId id)
{
    PRAGMA_ASSERT(m_document != nullptr, "DemoScene document must exist before clearing a script.");
    return m_document->ClearScript(id);
}

bool DemoScene::IsDocumentDirty() const noexcept
{
    return m_document != nullptr && m_document->IsDirty();
}

bool DemoScene::CanUndoDocument() const noexcept
{
    return m_document != nullptr && m_document->CanUndo();
}

bool DemoScene::CanRedoDocument() const noexcept
{
    return m_document != nullptr && m_document->CanRedo();
}

const std::string& DemoScene::GetUndoLabel() const noexcept
{
    static const std::string kEmpty;
    return m_document != nullptr ? m_document->GetUndoLabel() : kEmpty;
}

const std::string& DemoScene::GetRedoLabel() const noexcept
{
    static const std::string kEmpty;
    return m_document != nullptr ? m_document->GetRedoLabel() : kEmpty;
}

const std::string& DemoScene::GetLastHistoryAction() const noexcept
{
    static const std::string kEmpty;
    return m_document != nullptr ? m_document->GetLastHistoryAction() : kEmpty;
}

const std::filesystem::path& DemoScene::GetDocumentPath() const noexcept
{
    static const std::filesystem::path kEmptyPath;
    return m_document != nullptr ? m_document->GetPath() : kEmptyPath;
}

std::vector<std::string> DemoScene::GetAvailableScriptNames() const
{
    if (m_scriptRegistry == nullptr)
    {
        return {};
    }

    return m_scriptRegistry->GetRegisteredNames();
}

std::vector<Pragma::Renderer::NativeScriptMetadata> DemoScene::GetAvailableScripts() const
{
    if (m_scriptRegistry == nullptr)
    {
        return {};
    }

    return m_scriptRegistry->GetRegisteredScripts();
}

std::vector<Pragma::Scripting::ManagedScriptTypeMetadata> DemoScene::GetAvailableManagedScripts() const
{
    return m_managedScriptHost.GetAvailableScriptTypes();
}

std::vector<std::string> DemoScene::GetAvailableMaterialAssetNames() const
{
    std::vector<std::string> materialNames;
    for (const Pragma::Assets::AssetId& assetId : m_assets.GetAssetIdsByPrefix("material."))
    {
        materialNames.push_back(assetId.Value);
    }

    std::sort(materialNames.begin(), materialNames.end());
    return materialNames;
}

std::vector<std::string> DemoScene::GetAvailablePrefabAssetNames() const
{
    std::vector<std::string> prefabNames;
    for (const Pragma::Assets::AssetId& assetId : m_assets.GetAssetIdsByPrefix("prefab."))
    {
        prefabNames.push_back(assetId.Value);
    }

    std::sort(prefabNames.begin(), prefabNames.end());
    return prefabNames;
}

std::vector<std::string> DemoScene::GetAvailableTextureAssetNames() const
{
    std::vector<std::string> textureNames;
    for (const Pragma::Assets::AssetId& assetId : m_assets.GetAssetIdsByPrefix("texture."))
    {
        textureNames.push_back(assetId.Value);
    }

    std::sort(textureNames.begin(), textureNames.end());
    return textureNames;
}

std::filesystem::path DemoScene::ResolveAssetPath(const Pragma::Assets::AssetId& assetId) const
{
    return m_assets.ResolvePath(assetId);
}

const Pragma::Renderer::Scene& DemoScene::GetScene() const noexcept
{
    return m_document->GetScene();
}

Pragma::Renderer::Scene& DemoScene::GetScene() noexcept
{
    return m_document->GetScene();
}

void DemoScene::RegisterScripts()
{
    m_scriptRegistry = std::make_unique<Pragma::Renderer::NativeScriptRegistry>();
    m_scriptRegistry->Register<SpinScript>("Spin.Center", "Spin Center", "Rotates the center showcase cube clockwise.", 0.65f);
    m_scriptRegistry->Register<SpinScript>("Spin.Warm", "Spin Warm", "Rotates the warm showcase cube counter-clockwise.", -0.45f);
    m_scriptRegistry->Register<SpinScript>("Spin.Cool", "Spin Cool", "Rotates the cool showcase cube clockwise faster.", 0.85f);
}
}
