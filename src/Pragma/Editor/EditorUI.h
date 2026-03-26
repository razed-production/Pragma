#pragma once

#include "Pragma/Assets/MaterialAssetData.h"
#include "Pragma/Core/SceneObjectTemplate.h"
#include "Pragma/Renderer/Component.h"
#include "Pragma/Renderer/Entity.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace Pragma::Core
{
class DemoScene;
}

namespace Pragma::Physics
{
class PhysicsSystem;
}

namespace Pragma::Renderer
{
struct SceneObject;
}

namespace Pragma::Editor
{
class EditorUI
{
public:
    enum class GizmoMode
    {
        Move,
        Rotate,
        Scale
    };

    enum class GizmoAxis
    {
        None,
        X,
        Y,
        Z
    };

    struct Notification
    {
        std::string Message;
        float RemainingSeconds = 0.0f;
    };

    struct PendingCreateObjectRequest
    {
        std::string Name = "GameObject";
        Pragma::Core::SceneObjectTemplate ObjectTemplate = Pragma::Core::SceneObjectTemplate::Empty;
        Pragma::Renderer::EntityId Parent = Pragma::Renderer::InvalidEntityId;
        bool Pending = false;
    };

    struct PendingComponentRequest
    {
        Pragma::Renderer::EntityId Entity = Pragma::Renderer::InvalidEntityId;
        Pragma::Renderer::ComponentType Type = Pragma::Renderer::ComponentType::Transform;
        bool Pending = false;
    };

    struct PendingScriptRequest
    {
        Pragma::Renderer::EntityId Entity = Pragma::Renderer::InvalidEntityId;
        std::string ScriptName;
        bool Clear = false;
        bool Pending = false;
    };

    struct PendingManagedScriptRequest
    {
        Pragma::Renderer::EntityId Entity = Pragma::Renderer::InvalidEntityId;
        std::string ProjectAssetName;
        std::string TypeName;
        bool Pending = false;
    };

    struct PendingMaterialRequest
    {
        Pragma::Renderer::EntityId Entity = Pragma::Renderer::InvalidEntityId;
        std::string MaterialAssetName;
        bool Pending = false;
    };

    struct PendingMaterialAssetSaveRequest
    {
        std::string MaterialAssetName;
        Pragma::Assets::MaterialAssetData MaterialData;
        bool Pending = false;
    };

    struct PendingParentRequest
    {
        Pragma::Renderer::EntityId Child = Pragma::Renderer::InvalidEntityId;
        Pragma::Renderer::EntityId Parent = Pragma::Renderer::InvalidEntityId;
        bool Pending = false;
    };

    struct PendingPrefabInstantiateRequest
    {
        std::string PrefabAssetName;
        Pragma::Renderer::EntityId Parent = Pragma::Renderer::InvalidEntityId;
        bool Pending = false;
    };

    struct PendingPrefabSaveRequest
    {
        std::string PrefabAssetName;
        Pragma::Renderer::EntityId Root = Pragma::Renderer::InvalidEntityId;
        bool Pending = false;
    };

    struct PendingPrefabRevertRequest
    {
        Pragma::Renderer::EntityId Root = Pragma::Renderer::InvalidEntityId;
        bool Pending = false;
    };

    struct PendingPrefabApplyRequest
    {
        Pragma::Renderer::EntityId Root = Pragma::Renderer::InvalidEntityId;
        bool Pending = false;
    };

    EditorUI() = default;

    void BeginFrame(
        float deltaSeconds,
        Pragma::Core::DemoScene& demoScene,
        Pragma::Physics::PhysicsSystem& physicsSystem,
        bool& showDiagnosticsWindow,
        bool& showProfilerWindow,
        bool& showLogConsoleWindow);
    void ApplyPendingSceneActions(Pragma::Core::DemoScene& demoScene);

    [[nodiscard]] Pragma::Renderer::EntityId GetSelectedObjectId() const noexcept;
    [[nodiscard]] bool IsPhysicsOverlayEnabled() const noexcept;

private:
    struct LayoutState
    {
        bool ShowEditorWindow = true;
        bool ShowHierarchyWindow = true;
        bool ShowSceneViewWindow = true;
        bool ShowInspectorWindow = true;
        bool ShowMaterialBrowserWindow = true;
        bool ShowPrefabBrowserWindow = true;
        bool ShowPhysicsDebugWindow = true;
        bool ShowPhysicsOverlay = false;
        bool ShowNotificationsWindow = true;
        bool ShowStatusWindow = true;
        bool ShowDiagnosticsWindow = true;
        bool ShowProfilerWindow = true;
        bool ShowLogConsoleWindow = true;

        [[nodiscard]] bool operator==(const LayoutState& other) const noexcept = default;
    };

    void LoadLayoutState(
        bool& showDiagnosticsWindow,
        bool& showProfilerWindow,
        bool& showLogConsoleWindow);
    void SaveLayoutState(const LayoutState& state) const;
    [[nodiscard]] LayoutState CaptureLayoutState(
        bool showDiagnosticsWindow,
        bool showProfilerWindow,
        bool showLogConsoleWindow) const noexcept;
    void ResetLayoutState(
        bool& showDiagnosticsWindow,
        bool& showProfilerWindow,
        bool& showLogConsoleWindow);
    void OpenCreateObjectDialog(Pragma::Core::SceneObjectTemplate objectTemplate, Pragma::Renderer::EntityId parentId);
    void SyncObjectNameEditor(const Pragma::Renderer::SceneObject& object);
    void PushNotification(std::string message, float lifetimeSeconds = 3.0f);

private:
    Pragma::Renderer::EntityId m_selectedObjectId = Pragma::Renderer::InvalidEntityId;
    bool m_saveSceneRequested = false;
    bool m_reloadSceneRequested = false;
    bool m_undoRequested = false;
    bool m_redoRequested = false;
    PendingCreateObjectRequest m_createObjectRequest;
    Pragma::Renderer::EntityId m_duplicateObjectId = Pragma::Renderer::InvalidEntityId;
    Pragma::Renderer::EntityId m_deleteObjectId = Pragma::Renderer::InvalidEntityId;
    PendingComponentRequest m_addComponentRequest;
    PendingComponentRequest m_removeComponentRequest;
    PendingMaterialRequest m_materialRequest;
    PendingMaterialAssetSaveRequest m_materialAssetSaveRequest;
    PendingScriptRequest m_scriptRequest;
    PendingManagedScriptRequest m_managedScriptRequest;
    PendingParentRequest m_parentRequest;
    PendingPrefabInstantiateRequest m_prefabInstantiateRequest;
    PendingPrefabSaveRequest m_prefabSaveRequest;
    PendingPrefabRevertRequest m_prefabRevertRequest;
    PendingPrefabApplyRequest m_prefabApplyRequest;
    std::array<char, 256> m_objectNameBuffer{};
    std::array<char, 256> m_createObjectNameBuffer{};
    std::array<char, 128> m_hierarchyFilterBuffer{};
    std::array<char, 128> m_materialFilterBuffer{};
    std::array<char, 128> m_prefabFilterBuffer{};
    std::array<char, 128> m_prefabSaveNameBuffer{};
    Pragma::Renderer::EntityId m_objectNameBufferEntityId = Pragma::Renderer::InvalidEntityId;
    Pragma::Renderer::EntityId m_lastMaterialSyncEntityId = Pragma::Renderer::InvalidEntityId;
    Pragma::Renderer::EntityId m_createObjectParentId = Pragma::Renderer::InvalidEntityId;
    Pragma::Core::SceneObjectTemplate m_createObjectTemplate = Pragma::Core::SceneObjectTemplate::Empty;
    GizmoMode m_gizmoMode = GizmoMode::Move;
    GizmoAxis m_activeGizmoAxis = GizmoAxis::None;
    bool m_useLocalGizmoSpace = true;
    float m_lastGizmoMouseX = 0.0f;
    float m_lastGizmoMouseY = 0.0f;
    bool m_focusCreateObjectName = false;
    bool m_layoutLoaded = false;
    bool m_resetLayoutRequested = false;
    bool m_showEditorWindow = true;
    bool m_showHierarchyWindow = true;
    bool m_showSceneViewWindow = true;
    bool m_showInspectorWindow = true;
    bool m_showMaterialBrowserWindow = true;
    bool m_showPrefabBrowserWindow = true;
    bool m_showPhysicsDebugWindow = true;
    bool m_showPhysicsOverlay = false;
    bool m_showNotificationsWindow = true;
    bool m_showStatusWindow = true;
    LayoutState m_lastSavedLayoutState{};
    std::filesystem::path m_layoutStatePath;
    std::string m_selectedMaterialAssetName;
    std::string m_loadedMaterialEditorAssetName;
    std::string m_selectedPrefabAssetName;
    Pragma::Assets::MaterialAssetData m_materialEditorData{};
    std::vector<Notification> m_notifications;
};
}
