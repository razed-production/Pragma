#include "Pragma/Editor/EditorUI.h"

#include "Pragma/Core/DemoScene.h"
#include "Pragma/Math/Vector3.h"
#include "Pragma/Physics/PhysicsSystem.h"
#include "Pragma/Renderer/LightComponent.h"
#include "Pragma/Renderer/ManagedScriptComponent.h"
#include "Pragma/Renderer/NativeScriptComponent.h"
#include "Pragma/Renderer/Transform.h"
#include "imgui.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <limits>
#include <sstream>
#include <string>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace Pragma::Editor
{
namespace
{
constexpr float kRadiansToDegrees = 57.2957795131f;
constexpr float kDegreesToRadians = 0.01745329252f;

std::filesystem::path GetEditorSavedDirectory()
{
    wchar_t buffer[MAX_PATH]{};
    const DWORD length = GetModuleFileNameW(nullptr, buffer, static_cast<DWORD>(std::size(buffer)));
    std::filesystem::path directory = std::filesystem::path(std::wstring(buffer, length)).parent_path().parent_path().parent_path() / "saved";
    std::filesystem::create_directories(directory);
    return directory;
}

bool ParseBoolValue(const std::string& value)
{
    return value == "true" || value == "1";
}

const char* ToComponentLabel(const Pragma::Renderer::ComponentType type) noexcept
{
    using Pragma::Renderer::ComponentType;

    switch (type)
    {
    case ComponentType::Transform:
        return "Transform";
    case ComponentType::Camera:
        return "Camera";
    case ComponentType::CameraController:
        return "Camera Controller";
    case ComponentType::Light:
        return "Directional Light";
    case ComponentType::MeshRenderer:
        return "Mesh Renderer";
    case ComponentType::Behaviour:
        return "Native Script";
    case ComponentType::ManagedScript:
        return "Managed Script";
    case ComponentType::RigidBody:
        return "Rigid Body";
    case ComponentType::BoxCollider:
        return "Box Collider";
    case ComponentType::PrefabInstance:
        return "Prefab Instance";
    default:
        return "Component";
    }
}

const char* ToRigidBodyMotionTypeLabel(const Pragma::Renderer::RigidBodyMotionType motionType) noexcept
{
    switch (motionType)
    {
    case Pragma::Renderer::RigidBodyMotionType::Static:
        return "Static";
    case Pragma::Renderer::RigidBodyMotionType::Kinematic:
        return "Kinematic";
    case Pragma::Renderer::RigidBodyMotionType::Dynamic:
    default:
        return "Dynamic";
    }
}

const char* ToRigidBodyCollisionLayerLabel(const Pragma::Renderer::RigidBodyCollisionLayer collisionLayer) noexcept
{
    switch (collisionLayer)
    {
    case Pragma::Renderer::RigidBodyCollisionLayer::NoCollision:
        return "No Collision";
    case Pragma::Renderer::RigidBodyCollisionLayer::Default:
    default:
        return "Default";
    }
}

bool MatchesHierarchyFilter(const Pragma::Renderer::SceneObject& object, const char* filterText)
{
    if (filterText == nullptr || filterText[0] == '\0')
    {
        return true;
    }

    auto equalIgnoreCase = [](const char lhs, const char rhs)
    {
        return std::tolower(static_cast<unsigned char>(lhs)) == std::tolower(static_cast<unsigned char>(rhs));
    };

    const auto it = std::search(object.Name.begin(), object.Name.end(), filterText, filterText + std::strlen(filterText), equalIgnoreCase);
    return it != object.Name.end();
}

bool MatchesHierarchyFilterRecursive(
    const Pragma::Renderer::Scene& scene,
    const Pragma::Renderer::EntityId objectId,
    const char* filterText)
{
    const Pragma::Renderer::SceneObject* object = scene.FindObject(objectId);
    if (object == nullptr)
    {
        return false;
    }

    if (MatchesHierarchyFilter(*object, filterText))
    {
        return true;
    }

    for (const Pragma::Renderer::EntityId childId : scene.GetChildren(objectId))
    {
        if (MatchesHierarchyFilterRecursive(scene, childId, filterText))
        {
            return true;
        }
    }

    return false;
}

bool MatchesTextFilter(const std::string& value, const char* filterText)
{
    if (filterText == nullptr || filterText[0] == '\0')
    {
        return true;
    }

    auto equalIgnoreCase = [](const char lhs, const char rhs)
    {
        return std::tolower(static_cast<unsigned char>(lhs)) == std::tolower(static_cast<unsigned char>(rhs));
    };

    const auto it = std::search(value.begin(), value.end(), filterText, filterText + std::strlen(filterText), equalIgnoreCase);
    return it != value.end();
}

const char* ToObjectTemplateLabel(const Pragma::Core::SceneObjectTemplate objectTemplate) noexcept
{
    switch (objectTemplate)
    {
    case Pragma::Core::SceneObjectTemplate::Empty:
        return "Empty";
    case Pragma::Core::SceneObjectTemplate::Cube:
        return "Cube";
    case Pragma::Core::SceneObjectTemplate::Camera:
        return "Camera";
    case Pragma::Core::SceneObjectTemplate::DirectionalLight:
        return "Directional Light";
    case Pragma::Core::SceneObjectTemplate::PhysicsCube:
        return "Physics Cube";
    default:
        return "Object";
    }
}

std::string GetDefaultObjectName(const Pragma::Core::SceneObjectTemplate objectTemplate)
{
    switch (objectTemplate)
    {
    case Pragma::Core::SceneObjectTemplate::Empty:
        return "GameObject";
    case Pragma::Core::SceneObjectTemplate::Cube:
        return "Cube";
    case Pragma::Core::SceneObjectTemplate::Camera:
        return "Camera";
    case Pragma::Core::SceneObjectTemplate::DirectionalLight:
        return "Directional Light";
    case Pragma::Core::SceneObjectTemplate::PhysicsCube:
        return "Physics Cube";
    default:
        return "GameObject";
    }
}

std::string MakePrefabAssetNameFromObjectName(const std::string& objectName)
{
    std::string suffix;
    suffix.reserve(objectName.size());
    for (const char character : objectName)
    {
        if (std::isalnum(static_cast<unsigned char>(character)))
        {
            suffix.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(character))));
        }
        else if (character == ' ' || character == '-' || character == '.')
        {
            suffix.push_back('_');
        }
    }

    if (suffix.empty())
    {
        suffix = "new_prefab";
    }

    return "prefab." + suffix;
}

bool SceneHasDirectionalLight(const Pragma::Renderer::Scene& scene)
{
    for (const Pragma::Renderer::SceneObject& object : scene.GetObjects())
    {
        if (object.HasLight())
        {
            return true;
        }
    }

    return false;
}

bool SubtreeContainsDirectionalLight(const Pragma::Renderer::Scene& scene, const Pragma::Renderer::EntityId rootId)
{
    for (const Pragma::Renderer::EntityId objectId : scene.GetSubtree(rootId))
    {
        const Pragma::Renderer::SceneObject* object = scene.FindObject(objectId);
        if (object != nullptr && object->HasLight())
        {
            return true;
        }
    }

    return false;
}

bool HasInvalidBoxCollider(const Pragma::Renderer::BoxColliderComponent& boxCollider) noexcept
{
    return boxCollider.HalfExtent.X <= 0.0f || boxCollider.HalfExtent.Y <= 0.0f || boxCollider.HalfExtent.Z <= 0.0f;
}

template <std::size_t TSize>
void CopyTextToBuffer(std::array<char, TSize>& buffer, const std::string& value)
{
    buffer.fill('\0');
    strncpy_s(buffer.data(), buffer.size(), value.c_str(), _TRUNCATE);
}

Pragma::Math::Vector3 GetCameraForward(const float yawRadians, const float pitchRadians) noexcept
{
    Pragma::Renderer::Transform cameraTransform{};
    cameraTransform.RotationRadians = { pitchRadians, yawRadians, 0.0f };
    return Pragma::Math::Normalize(Pragma::Renderer::GetForwardAxis(cameraTransform));
}
}

void EditorUI::BeginFrame(
    const float deltaSeconds,
    Pragma::Core::DemoScene& demoScene,
    Pragma::Physics::PhysicsSystem& physicsSystem,
    bool& showDiagnosticsWindow,
    bool& showProfilerWindow,
    bool& showLogConsoleWindow)
{
    if (!m_layoutLoaded)
    {
        LoadLayoutState(showDiagnosticsWindow, showProfilerWindow, showLogConsoleWindow);
    }

    Pragma::Renderer::Scene& scene = demoScene.GetScene();
    if (m_selectedObjectId != Pragma::Renderer::InvalidEntityId && !scene.IsEntityAlive(m_selectedObjectId))
    {
        m_selectedObjectId = Pragma::Renderer::InvalidEntityId;
        m_objectNameBufferEntityId = Pragma::Renderer::InvalidEntityId;
        m_objectNameBuffer.fill('\0');
    }

    if (const Pragma::Renderer::SceneObject* selectedObject = scene.FindObject(m_selectedObjectId))
    {
        SyncObjectNameEditor(*selectedObject);
        if (selectedObject->Id != m_lastMaterialSyncEntityId)
        {
            m_lastMaterialSyncEntityId = selectedObject->Id;
            if (const auto* meshRenderer = selectedObject->GetMeshRenderer(); meshRenderer != nullptr)
            {
                m_selectedMaterialAssetName = meshRenderer->MaterialAssetId.Value;
            }
        }
    }
    else
    {
        m_lastMaterialSyncEntityId = Pragma::Renderer::InvalidEntityId;
    }

    if (!m_selectedMaterialAssetName.empty() && m_loadedMaterialEditorAssetName != m_selectedMaterialAssetName)
    {
        m_materialEditorData = demoScene.LoadMaterialAssetData({ m_selectedMaterialAssetName });
        m_loadedMaterialEditorAssetName = m_selectedMaterialAssetName;
    }

    const bool sceneHasDirectionalLight = SceneHasDirectionalLight(scene);

    auto frameObjectSelection = [&](const Pragma::Renderer::EntityId targetId) -> bool
    {
        Pragma::Renderer::SceneObject* activeCameraObject = scene.GetActiveCameraObject();
        const Pragma::Renderer::SceneObject* targetObject = scene.FindObject(targetId);
        if (activeCameraObject == nullptr || !activeCameraObject->HasCamera() || targetObject == nullptr)
        {
            return false;
        }

        if (activeCameraObject->Id == targetId)
        {
            PushNotification("Active camera cannot frame itself.");
            return false;
        }

        Pragma::Renderer::CameraComponent* activeCamera = activeCameraObject->GetCamera();
        if (activeCamera == nullptr)
        {
            return false;
        }

        const Pragma::Renderer::Transform targetWorldTransform = scene.GetWorldTransform(targetId);
        Pragma::Renderer::Transform cameraWorldTransform = scene.GetWorldTransform(activeCameraObject->Id);

        Pragma::Math::Vector3 toCamera = cameraWorldTransform.Position - targetWorldTransform.Position;
        const float toCameraLength = Pragma::Math::Length(toCamera);
        Pragma::Math::Vector3 directionFromTargetToCamera =
            toCameraLength > 0.001f
                ? Pragma::Math::Normalize(toCamera)
                : GetCameraForward(cameraWorldTransform.RotationRadians.Y, activeCamera->PitchRadians) * -1.0f;

        const float focusRadius = std::max(
            3.0f,
            std::max({ std::abs(targetWorldTransform.Scale.X), std::abs(targetWorldTransform.Scale.Y), std::abs(targetWorldTransform.Scale.Z) }) * 3.0f);
        const Pragma::Math::Vector3 newCameraPosition = targetWorldTransform.Position + directionFromTargetToCamera * focusRadius;
        const Pragma::Math::Vector3 lookDirection = Pragma::Math::Normalize(targetWorldTransform.Position - newCameraPosition);
        if (Pragma::Math::Length(lookDirection) <= 0.0001f)
        {
            return false;
        }

        demoScene.CaptureUndoState("Frame Selected");
        cameraWorldTransform.Position = newCameraPosition;
        cameraWorldTransform.RotationRadians.X = 0.0f;
        cameraWorldTransform.RotationRadians.Y = std::atan2(lookDirection.X, lookDirection.Z);
        cameraWorldTransform.RotationRadians.Z = 0.0f;
        activeCamera->PitchRadians = std::asin(std::clamp(lookDirection.Y, -1.0f, 1.0f));

        if (!scene.SetWorldTransform(activeCameraObject->Id, cameraWorldTransform))
        {
            return false;
        }

        demoScene.MarkDocumentDirty();
        PushNotification("Framed '" + targetObject->Name + "'.");
        return true;
    };

    for (auto it = m_notifications.begin(); it != m_notifications.end();)
    {
        it->RemainingSeconds -= deltaSeconds;
        if (it->RemainingSeconds <= 0.0f)
        {
            it = m_notifications.erase(it);
        }
        else
        {
            ++it;
        }
    }

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
            {
                m_saveSceneRequested = true;
            }

            if (ImGui::MenuItem("Reload Scene"))
            {
                m_reloadSceneRequested = true;
            }

            ImGui::Separator();
            if (ImGui::BeginMenu("Create"))
            {
                if (ImGui::MenuItem("Empty"))
                {
                    OpenCreateObjectDialog(Pragma::Core::SceneObjectTemplate::Empty, Pragma::Renderer::InvalidEntityId);
                }
                if (ImGui::MenuItem("Cube"))
                {
                    OpenCreateObjectDialog(Pragma::Core::SceneObjectTemplate::Cube, Pragma::Renderer::InvalidEntityId);
                }
                if (ImGui::MenuItem("Camera"))
                {
                    OpenCreateObjectDialog(Pragma::Core::SceneObjectTemplate::Camera, Pragma::Renderer::InvalidEntityId);
                }
                if (sceneHasDirectionalLight)
                {
                    ImGui::BeginDisabled();
                }
                if (ImGui::MenuItem("Directional Light"))
                {
                    OpenCreateObjectDialog(Pragma::Core::SceneObjectTemplate::DirectionalLight, Pragma::Renderer::InvalidEntityId);
                }
                if (sceneHasDirectionalLight)
                {
                    ImGui::EndDisabled();
                }
                if (ImGui::MenuItem("Physics Cube"))
                {
                    OpenCreateObjectDialog(Pragma::Core::SceneObjectTemplate::PhysicsCube, Pragma::Renderer::InvalidEntityId);
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem("Undo", "Ctrl+Z", false, demoScene.CanUndoDocument()))
            {
                m_undoRequested = true;
            }

            if (ImGui::MenuItem("Redo", "Ctrl+Y", false, demoScene.CanRedoDocument()))
            {
                m_redoRequested = true;
            }

            ImGui::Separator();
            if (ImGui::MenuItem("Duplicate Selected", "Ctrl+D", false, m_selectedObjectId != Pragma::Renderer::InvalidEntityId))
            {
                m_duplicateObjectId = m_selectedObjectId;
            }

            if (ImGui::MenuItem("Delete Selected", "Del", false, m_selectedObjectId != Pragma::Renderer::InvalidEntityId))
            {
                m_deleteObjectId = m_selectedObjectId;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View"))
        {
            ImGui::MenuItem("Editor", nullptr, &m_showEditorWindow);
            ImGui::MenuItem("Hierarchy", nullptr, &m_showHierarchyWindow);
            ImGui::MenuItem("Scene View", nullptr, &m_showSceneViewWindow);
            ImGui::MenuItem("Inspector", nullptr, &m_showInspectorWindow);
            ImGui::MenuItem("Material Browser", nullptr, &m_showMaterialBrowserWindow);
            ImGui::MenuItem("Prefab Browser", nullptr, &m_showPrefabBrowserWindow);
            ImGui::MenuItem("Physics Debug", nullptr, &m_showPhysicsDebugWindow);
            ImGui::MenuItem("Physics Overlay", nullptr, &m_showPhysicsOverlay);
            ImGui::MenuItem("Notifications", nullptr, &m_showNotificationsWindow);
            ImGui::MenuItem("Status", nullptr, &m_showStatusWindow);
            ImGui::Separator();
            if (ImGui::MenuItem("Reset Layout"))
            {
                m_resetLayoutRequested = true;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Tools"))
        {
            ImGui::MenuItem("Diagnostics", nullptr, &showDiagnosticsWindow);
            ImGui::MenuItem("Profiler", nullptr, &showProfilerWindow);
            ImGui::MenuItem("Log Console", nullptr, &showLogConsoleWindow);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help"))
        {
            ImGui::TextUnformatted("Pragma Editor");
            ImGui::Separator();
            ImGui::TextDisabled("Hierarchy selects objects");
            ImGui::TextDisabled("Scene View holds scene tools");
            ImGui::TextDisabled("Inspector edits properties");
            ImGui::TextDisabled("View/Tools toggle editor and debug panels");
            ImGui::TextDisabled("F frames selected object");
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    const ImGuiIO& io = ImGui::GetIO();
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false))
    {
        m_saveSceneRequested = true;
    }
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_D, false) && m_selectedObjectId != Pragma::Renderer::InvalidEntityId)
    {
        m_duplicateObjectId = m_selectedObjectId;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Delete, false) && m_selectedObjectId != Pragma::Renderer::InvalidEntityId)
    {
        m_deleteObjectId = m_selectedObjectId;
    }

    if (m_showEditorWindow)
    {
        ImGui::SetNextWindowSize(ImVec2(420.0f, 140.0f), ImGuiCond_FirstUseEver);
        ImGui::Begin("Editor", &m_showEditorWindow);
        const std::string documentPath = demoScene.GetDocumentPath().string();
        ImGui::TextWrapped("Document: %s", documentPath.empty() ? "<none>" : documentPath.c_str());
        ImGui::Text("Dirty: %s", demoScene.IsDocumentDirty() ? "Yes" : "No");
        ImGui::TextWrapped(
            "Last Action: %s",
            demoScene.GetLastHistoryAction().empty() ? "None" : demoScene.GetLastHistoryAction().c_str());

        const bool canUndo = demoScene.CanUndoDocument();
        const bool canRedo = demoScene.CanRedoDocument();
        if (!canUndo)
        {
            ImGui::BeginDisabled();
        }
        if (ImGui::Button("Undo"))
        {
            m_undoRequested = true;
        }
        if (!canUndo)
        {
            ImGui::EndDisabled();
        }
        if (ImGui::IsItemHovered() && canUndo)
        {
            ImGui::SetTooltip("Undo %s", demoScene.GetUndoLabel().c_str());
        }

        ImGui::SameLine();
        if (!canRedo)
        {
            ImGui::BeginDisabled();
        }
        if (ImGui::Button("Redo"))
        {
            m_redoRequested = true;
        }
        if (!canRedo)
        {
            ImGui::EndDisabled();
        }
        if (ImGui::IsItemHovered() && canRedo)
        {
            ImGui::SetTooltip("Redo %s", demoScene.GetRedoLabel().c_str());
        }

        ImGui::SameLine();
        if (ImGui::Button("Save"))
        {
            m_saveSceneRequested = true;
        }

        ImGui::SameLine();
        if (ImGui::Button("Reload"))
        {
            m_reloadSceneRequested = true;
        }

        if (ImGui::Button("Create"))
        {
            OpenCreateObjectDialog(Pragma::Core::SceneObjectTemplate::Empty, Pragma::Renderer::InvalidEntityId);
        }

        ImGui::SameLine();
        ImGui::TextDisabled("Ctrl+Z / Ctrl+Y");
        ImGui::End();
    }

    if (m_showHierarchyWindow)
    {
        ImGui::SetNextWindowSize(ImVec2(320.0f, 520.0f), ImGuiCond_FirstUseEver);
        ImGui::Begin("Hierarchy", &m_showHierarchyWindow);
        ImGui::InputTextWithHint("##HierarchyFilter", "Filter objects...", m_hierarchyFilterBuffer.data(), m_hierarchyFilterBuffer.size());
        ImGui::Separator();

        std::function<void(Pragma::Renderer::EntityId)> drawHierarchyNode =
            [&](const Pragma::Renderer::EntityId objectId)
        {
            const Pragma::Renderer::SceneObject* object = scene.FindObject(objectId);
            if (object == nullptr)
            {
                return;
            }

            const char* filterText = m_hierarchyFilterBuffer.data();
            if (!MatchesHierarchyFilterRecursive(scene, objectId, filterText))
            {
                return;
            }

            const std::vector<Pragma::Renderer::EntityId> children = scene.GetChildren(objectId);
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
            if (children.empty())
            {
                flags |= ImGuiTreeNodeFlags_Leaf;
            }
            if (m_selectedObjectId == objectId)
            {
                flags |= ImGuiTreeNodeFlags_Selected;
            }

            std::string displayName = object->Name;
            if (object->HasPrefabInstance())
            {
                displayName += demoScene.HasPrefabOverrides(objectId) ? " [Prefab*]" : " [Prefab]";
            }

            const bool open = ImGui::TreeNodeEx(
                reinterpret_cast<void*>(static_cast<std::uintptr_t>(objectId)),
                flags,
                "%s##%llu",
                displayName.c_str(),
                static_cast<unsigned long long>(objectId));

            if (ImGui::IsItemClicked())
            {
                m_selectedObjectId = objectId;
            }

            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                m_selectedObjectId = objectId;
                (void)frameObjectSelection(objectId);
            }

            if (ImGui::BeginDragDropSource())
            {
                ImGui::SetDragDropPayload("HierarchyEntity", &objectId, sizeof(objectId));
                ImGui::TextUnformatted(object->Name.c_str());
                ImGui::EndDragDropSource();
            }

            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HierarchyEntity"))
                {
                    const auto draggedId = *static_cast<const Pragma::Renderer::EntityId*>(payload->Data);
                    if (draggedId != objectId)
                    {
                        m_parentRequest.Child = draggedId;
                        m_parentRequest.Parent = objectId;
                        m_parentRequest.Pending = true;
                    }
                }
                ImGui::EndDragDropTarget();
            }

            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::MenuItem("Create Child"))
                {
                    OpenCreateObjectDialog(Pragma::Core::SceneObjectTemplate::Empty, objectId);
                }
                if (ImGui::BeginMenu("Create Child As"))
                {
                    if (ImGui::MenuItem("Cube"))
                    {
                        OpenCreateObjectDialog(Pragma::Core::SceneObjectTemplate::Cube, objectId);
                    }
                    if (ImGui::MenuItem("Camera"))
                    {
                        OpenCreateObjectDialog(Pragma::Core::SceneObjectTemplate::Camera, objectId);
                    }
                    if (sceneHasDirectionalLight)
                    {
                        ImGui::BeginDisabled();
                    }
                    if (ImGui::MenuItem("Directional Light"))
                    {
                        OpenCreateObjectDialog(Pragma::Core::SceneObjectTemplate::DirectionalLight, objectId);
                    }
                    if (sceneHasDirectionalLight)
                    {
                        ImGui::EndDisabled();
                    }
                    if (ImGui::MenuItem("Physics Cube"))
                    {
                        OpenCreateObjectDialog(Pragma::Core::SceneObjectTemplate::PhysicsCube, objectId);
                    }
                    ImGui::EndMenu();
                }

                if (object->ParentId != Pragma::Renderer::InvalidEntityId && ImGui::MenuItem("Unparent"))
                {
                    m_parentRequest.Child = objectId;
                    m_parentRequest.Parent = Pragma::Renderer::InvalidEntityId;
                    m_parentRequest.Pending = true;
                }

                if (ImGui::MenuItem("Duplicate"))
                {
                    m_duplicateObjectId = objectId;
                }

                if (ImGui::MenuItem("Delete"))
                {
                    m_deleteObjectId = objectId;
                }

                ImGui::EndPopup();
            }

            if (open)
            {
                for (const Pragma::Renderer::EntityId childId : children)
                {
                    drawHierarchyNode(childId);
                }
                ImGui::TreePop();
            }
        };

        for (const Pragma::Renderer::SceneObject& object : scene.GetObjects())
        {
            if (object.ParentId == Pragma::Renderer::InvalidEntityId)
            {
                drawHierarchyNode(object.Id);
            }
        }

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HierarchyEntity"))
            {
                const auto draggedId = *static_cast<const Pragma::Renderer::EntityId*>(payload->Data);
                m_parentRequest.Child = draggedId;
                m_parentRequest.Parent = Pragma::Renderer::InvalidEntityId;
                m_parentRequest.Pending = true;
            }
            ImGui::EndDragDropTarget();
        }

        if (ImGui::BeginPopupContextWindow("HierarchyContextMenu", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
        {
            if (ImGui::MenuItem("Create Empty"))
            {
                OpenCreateObjectDialog(Pragma::Core::SceneObjectTemplate::Empty, Pragma::Renderer::InvalidEntityId);
            }
            if (ImGui::MenuItem("Create Cube"))
            {
                OpenCreateObjectDialog(Pragma::Core::SceneObjectTemplate::Cube, Pragma::Renderer::InvalidEntityId);
            }
            if (ImGui::MenuItem("Create Camera"))
            {
                OpenCreateObjectDialog(Pragma::Core::SceneObjectTemplate::Camera, Pragma::Renderer::InvalidEntityId);
            }
            if (sceneHasDirectionalLight)
            {
                ImGui::BeginDisabled();
            }
            if (ImGui::MenuItem("Create Directional Light"))
            {
                OpenCreateObjectDialog(Pragma::Core::SceneObjectTemplate::DirectionalLight, Pragma::Renderer::InvalidEntityId);
            }
            if (sceneHasDirectionalLight)
            {
                ImGui::EndDisabled();
            }
            if (ImGui::MenuItem("Create Physics Cube"))
            {
                OpenCreateObjectDialog(Pragma::Core::SceneObjectTemplate::PhysicsCube, Pragma::Renderer::InvalidEntityId);
            }
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopupModal("Create Object", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            int selectedTemplate = static_cast<int>(m_createObjectTemplate);
            constexpr std::array<const char*, 5> kTemplateLabels
            {
                "Empty",
                "Cube",
                "Camera",
                "Directional Light",
                "Physics Cube"
            };
            ImGui::Combo("Template", &selectedTemplate, kTemplateLabels.data(), static_cast<int>(kTemplateLabels.size()));
            const auto updatedTemplate = static_cast<Pragma::Core::SceneObjectTemplate>(selectedTemplate);
            if (updatedTemplate != m_createObjectTemplate)
            {
                m_createObjectTemplate = updatedTemplate;
                CopyTextToBuffer(m_createObjectNameBuffer, GetDefaultObjectName(m_createObjectTemplate));
                m_focusCreateObjectName = true;
            }

            ImGui::InputText("Name", m_createObjectNameBuffer.data(), m_createObjectNameBuffer.size());
            if (m_focusCreateObjectName)
            {
                ImGui::SetKeyboardFocusHere(-1);
                m_focusCreateObjectName = false;
            }

            const Pragma::Renderer::SceneObject* parentObject = scene.FindObject(m_createObjectParentId);
            ImGui::TextWrapped("Parent: %s", parentObject != nullptr ? parentObject->Name.c_str() : "<Root>");

            const bool createPressed = ImGui::Button("Create");
            ImGui::SameLine();
            const bool cancelPressed = ImGui::Button("Cancel");

            if (createPressed)
            {
                m_createObjectRequest.Name = m_createObjectNameBuffer.data();
                if (m_createObjectRequest.Name.empty())
                {
                    m_createObjectRequest.Name = GetDefaultObjectName(m_createObjectTemplate);
                }
                m_createObjectRequest.ObjectTemplate = m_createObjectTemplate;
                m_createObjectRequest.Parent = m_createObjectParentId;
                m_createObjectRequest.Pending = true;
                ImGui::CloseCurrentPopup();
            }

            if (cancelPressed)
            {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
        ImGui::End();
    }

    if (m_showSceneViewWindow)
    {
        ImGui::SetNextWindowSize(ImVec2(460.0f, 240.0f), ImGuiCond_FirstUseEver);
        ImGui::Begin("Scene View", &m_showSceneViewWindow);
        ImGui::TextWrapped("Viewport hidden in this layout. Use the main engine window for rendering and Hierarchy to choose the active object.");
        ImGui::Separator();

        if (ImGui::RadioButton("Move", m_gizmoMode == GizmoMode::Move))
        {
            m_gizmoMode = GizmoMode::Move;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Rotate", m_gizmoMode == GizmoMode::Rotate))
        {
            m_gizmoMode = GizmoMode::Rotate;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Scale", m_gizmoMode == GizmoMode::Scale))
        {
            m_gizmoMode = GizmoMode::Scale;
        }
        ImGui::SameLine();
        ImGui::Checkbox("Local Space", &m_useLocalGizmoSpace);
        ImGui::Checkbox("Physics Overlay", &m_showPhysicsOverlay);

        ImGui::Separator();

        const Pragma::Renderer::SceneObject* sceneViewSelection = scene.FindObject(m_selectedObjectId);
        if (sceneViewSelection == nullptr)
        {
            ImGui::TextDisabled("No object selected.");
        }
        else
        {
            if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsKeyPressed(ImGuiKey_F, false))
            {
                (void)frameObjectSelection(sceneViewSelection->Id);
            }

            ImGui::Text("EntityId: %llu", static_cast<unsigned long long>(sceneViewSelection->Id));
            ImGui::Text("Selected: %s", sceneViewSelection->Name.c_str());
            if (ImGui::Button("Frame Selected"))
            {
                (void)frameObjectSelection(sceneViewSelection->Id);
            }

            const Pragma::Renderer::SceneObject* parentObject = scene.FindObject(sceneViewSelection->ParentId);
            ImGui::Text(
                "Parent: %s",
                sceneViewSelection->ParentId == Pragma::Renderer::InvalidEntityId
                    ? "<Root>"
                    : (parentObject != nullptr ? parentObject->Name.c_str() : "<Missing>"));
            ImGui::Text("Children: %llu", static_cast<unsigned long long>(scene.GetChildren(sceneViewSelection->Id).size()));
            ImGui::TextDisabled("Detailed properties live in Inspector.");
        }
        ImGui::End();
    }

    if (m_showInspectorWindow)
    {
        ImGui::SetNextWindowSize(ImVec2(460.0f, 720.0f), ImGuiCond_FirstUseEver);
        ImGui::Begin("Inspector", &m_showInspectorWindow);
        ImGui::TextDisabled("Object properties");
        ImGui::Separator();

        Pragma::Renderer::SceneObject* selectedObject = scene.FindObject(m_selectedObjectId);
        if (selectedObject == nullptr)
        {
            ImGui::TextDisabled("Select an object in Hierarchy to edit it here.");
        }
        else
        {
            if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsKeyPressed(ImGuiKey_F, false))
            {
                (void)frameObjectSelection(selectedObject->Id);
            }

            SyncObjectNameEditor(*selectedObject);
            ImGui::Text("EntityId: %llu", static_cast<unsigned long long>(selectedObject->Id));
            ImGui::Text("Selected: %s", selectedObject->Name.c_str());
            if (ImGui::Button("Frame Selected"))
            {
                (void)frameObjectSelection(selectedObject->Id);
            }
            ImGui::Separator();

            ImGui::InputText("Name", m_objectNameBuffer.data(), m_objectNameBuffer.size(), ImGuiInputTextFlags_EnterReturnsTrue);
            ImGui::SameLine();
            if (ImGui::Button("Apply Name"))
            {
                const std::string newName = m_objectNameBuffer.data();
                if (!newName.empty() && newName != selectedObject->Name && demoScene.RenameObject(selectedObject->Id, newName))
                {
                    PushNotification("Renamed object to '" + newName + "'.");
                }
            }

            const Pragma::Renderer::SceneObject* parentObject = scene.FindObject(selectedObject->ParentId);
            const char* parentLabel =
                selectedObject->ParentId == Pragma::Renderer::InvalidEntityId ? "<Root>" :
                (parentObject != nullptr ? parentObject->Name.c_str() : "<Missing>");
            if (ImGui::BeginCombo("Parent", parentLabel))
            {
                if (ImGui::Selectable("<Root>", selectedObject->ParentId == Pragma::Renderer::InvalidEntityId))
                {
                    m_parentRequest.Child = selectedObject->Id;
                    m_parentRequest.Parent = Pragma::Renderer::InvalidEntityId;
                    m_parentRequest.Pending = true;
                }

                for (const Pragma::Renderer::SceneObject& candidate : scene.GetObjects())
                {
                    if (candidate.Id == selectedObject->Id || scene.IsDescendant(candidate.Id, selectedObject->Id))
                    {
                        continue;
                    }

                    if (ImGui::Selectable(candidate.Name.c_str(), candidate.Id == selectedObject->ParentId))
                    {
                        m_parentRequest.Child = selectedObject->Id;
                        m_parentRequest.Parent = candidate.Id;
                        m_parentRequest.Pending = true;
                    }
                }

                ImGui::EndCombo();
            }

            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
            {
                std::array<float, 3> position
                {
                    selectedObject->Transform.Position.X,
                    selectedObject->Transform.Position.Y,
                    selectedObject->Transform.Position.Z
                };
                std::array<float, 3> rotationDegrees
                {
                    selectedObject->Transform.RotationRadians.X * kRadiansToDegrees,
                    selectedObject->Transform.RotationRadians.Y * kRadiansToDegrees,
                    selectedObject->Transform.RotationRadians.Z * kRadiansToDegrees
                };
                std::array<float, 3> scale
                {
                    selectedObject->Transform.Scale.X,
                    selectedObject->Transform.Scale.Y,
                    selectedObject->Transform.Scale.Z
                };
                const Pragma::Renderer::Transform worldTransform = scene.GetWorldTransform(selectedObject->Id);
                std::array<float, 3> worldPosition
                {
                    worldTransform.Position.X,
                    worldTransform.Position.Y,
                    worldTransform.Position.Z
                };
                std::array<float, 3> worldRotationDegrees
                {
                    worldTransform.RotationRadians.X * kRadiansToDegrees,
                    worldTransform.RotationRadians.Y * kRadiansToDegrees,
                    worldTransform.RotationRadians.Z * kRadiansToDegrees
                };
                std::array<float, 3> worldScale
                {
                    worldTransform.Scale.X,
                    worldTransform.Scale.Y,
                    worldTransform.Scale.Z
                };

                if (ImGui::DragFloat3("Position", position.data(), 0.05f))
                {
                    if (ImGui::IsItemActivated())
                    {
                        demoScene.CaptureUndoState("Edit Transform");
                    }
                    selectedObject->Transform.Position = { position[0], position[1], position[2] };
                    demoScene.MarkDocumentDirty();
                }

                if (ImGui::DragFloat3("Rotation", rotationDegrees.data(), 0.25f))
                {
                    if (ImGui::IsItemActivated())
                    {
                        demoScene.CaptureUndoState("Edit Transform");
                    }
                    selectedObject->Transform.RotationRadians =
                    {
                        rotationDegrees[0] * kDegreesToRadians,
                        rotationDegrees[1] * kDegreesToRadians,
                        rotationDegrees[2] * kDegreesToRadians
                    };
                    demoScene.MarkDocumentDirty();
                }

                if (ImGui::DragFloat3("Scale", scale.data(), 0.01f, 0.001f, 100.0f))
                {
                    if (ImGui::IsItemActivated())
                    {
                        demoScene.CaptureUndoState("Edit Transform");
                    }
                    selectedObject->Transform.Scale = { scale[0], scale[1], scale[2] };
                    demoScene.MarkDocumentDirty();
                }

                ImGui::Separator();
                ImGui::TextUnformatted("World Transform");
                if (ImGui::DragFloat3("World Position", worldPosition.data(), 0.05f))
                {
                    if (ImGui::IsItemActivated())
                    {
                        demoScene.CaptureUndoState("Edit World Transform");
                    }

                    Pragma::Renderer::Transform updatedWorld = worldTransform;
                    updatedWorld.Position = { worldPosition[0], worldPosition[1], worldPosition[2] };
                    if (scene.SetWorldTransform(selectedObject->Id, updatedWorld))
                    {
                        demoScene.MarkDocumentDirty();
                    }
                }

                if (ImGui::DragFloat3("World Rotation", worldRotationDegrees.data(), 0.25f))
                {
                    if (ImGui::IsItemActivated())
                    {
                        demoScene.CaptureUndoState("Edit World Transform");
                    }

                    Pragma::Renderer::Transform updatedWorld = worldTransform;
                    updatedWorld.RotationRadians =
                    {
                        worldRotationDegrees[0] * kDegreesToRadians,
                        worldRotationDegrees[1] * kDegreesToRadians,
                        worldRotationDegrees[2] * kDegreesToRadians
                    };

                    if (scene.SetWorldTransform(selectedObject->Id, updatedWorld))
                    {
                        demoScene.MarkDocumentDirty();
                    }
                }

                if (ImGui::DragFloat3("World Scale", worldScale.data(), 0.01f, 0.001f, 100.0f))
                {
                    if (ImGui::IsItemActivated())
                    {
                        demoScene.CaptureUndoState("Edit World Transform");
                    }

                    Pragma::Renderer::Transform updatedWorld = worldTransform;
                    updatedWorld.Scale = { worldScale[0], worldScale[1], worldScale[2] };
                    if (scene.SetWorldTransform(selectedObject->Id, updatedWorld))
                    {
                        demoScene.MarkDocumentDirty();
                    }
                }

                ImGui::Separator();
                ImGui::Text("World Position: %.2f %.2f %.2f", worldTransform.Position.X, worldTransform.Position.Y, worldTransform.Position.Z);
                ImGui::Text(
                    "World Rotation: %.1f %.1f %.1f",
                    worldTransform.RotationRadians.X * kRadiansToDegrees,
                    worldTransform.RotationRadians.Y * kRadiansToDegrees,
                    worldTransform.RotationRadians.Z * kRadiansToDegrees);
                ImGui::Text("World Scale: %.2f %.2f %.2f", worldTransform.Scale.X, worldTransform.Scale.Y, worldTransform.Scale.Z);
            }

            if (selectedObject->HasCamera() && ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
            {
                Pragma::Renderer::CameraComponent* camera = selectedObject->GetCamera();
                float pitchDegrees = camera->PitchRadians * kRadiansToDegrees;
                float fovDegrees = camera->FieldOfViewRadians * kRadiansToDegrees;

                if (scene.GetActiveCameraEntityId() != selectedObject->Id)
                {
                    if (ImGui::Button("Set Active Camera"))
                    {
                        demoScene.CaptureUndoState("Set Active Camera");
                        if (scene.SetActiveCamera(selectedObject->Id))
                        {
                            demoScene.MarkDocumentDirty();
                            PushNotification("Active camera changed to '" + selectedObject->Name + "'.");
                        }
                    }
                }
                else
                {
                    ImGui::TextDisabled("This is the active camera.");
                }

                if (ImGui::DragFloat("Pitch", &pitchDegrees, 0.25f, -89.0f, 89.0f))
                {
                    if (ImGui::IsItemActivated())
                    {
                        demoScene.CaptureUndoState("Edit Camera");
                    }
                    camera->PitchRadians = pitchDegrees * kDegreesToRadians;
                    demoScene.MarkDocumentDirty();
                }

                if (ImGui::DragFloat("Field Of View", &fovDegrees, 0.25f, 10.0f, 160.0f))
                {
                    if (ImGui::IsItemActivated())
                    {
                        demoScene.CaptureUndoState("Edit Camera");
                    }
                    camera->FieldOfViewRadians = fovDegrees * kDegreesToRadians;
                    demoScene.MarkDocumentDirty();
                }

                if (ImGui::DragFloat("Near Plane", &camera->NearPlane, 0.01f, 0.001f, camera->FarPlane))
                {
                    if (ImGui::IsItemActivated())
                    {
                        demoScene.CaptureUndoState("Edit Camera");
                    }
                    demoScene.MarkDocumentDirty();
                }

                if (ImGui::DragFloat("Far Plane", &camera->FarPlane, 0.1f, camera->NearPlane + 0.01f, 10000.0f))
                {
                    if (ImGui::IsItemActivated())
                    {
                        demoScene.CaptureUndoState("Edit Camera");
                    }
                    demoScene.MarkDocumentDirty();
                }

                if (ImGui::Button("Remove Camera"))
                {
                    m_removeComponentRequest = { selectedObject->Id, Pragma::Renderer::ComponentType::Camera, true };
                }
            }

            if (selectedObject->HasCameraController() && ImGui::CollapsingHeader("Camera Controller", ImGuiTreeNodeFlags_DefaultOpen))
            {
                Pragma::Renderer::CameraControllerComponent* controller = selectedObject->GetCameraController();
                if (ImGui::Checkbox("Enabled", &controller->Enabled))
                {
                    demoScene.CaptureUndoState("Edit Camera Controller");
                    demoScene.MarkDocumentDirty();
                }

                if (ImGui::DragFloat("Move Speed", &controller->MoveSpeed, 0.05f, 0.1f, 100.0f))
                {
                    if (ImGui::IsItemActivated())
                    {
                        demoScene.CaptureUndoState("Edit Camera Controller");
                    }
                    demoScene.MarkDocumentDirty();
                }

                if (ImGui::DragFloat("Fast Move Speed", &controller->FastMoveSpeed, 0.05f, 0.1f, 200.0f))
                {
                    if (ImGui::IsItemActivated())
                    {
                        demoScene.CaptureUndoState("Edit Camera Controller");
                    }
                    demoScene.MarkDocumentDirty();
                }

                if (ImGui::DragFloat("Look Speed", &controller->KeyboardLookSpeed, 0.01f, 0.01f, 10.0f))
                {
                    if (ImGui::IsItemActivated())
                    {
                        demoScene.CaptureUndoState("Edit Camera Controller");
                    }
                    demoScene.MarkDocumentDirty();
                }

                if (ImGui::DragFloat("Mouse Sensitivity", &controller->MouseLookSensitivity, 0.0001f, 0.0001f, 0.05f, "%.4f"))
                {
                    if (ImGui::IsItemActivated())
                    {
                        demoScene.CaptureUndoState("Edit Camera Controller");
                    }
                    demoScene.MarkDocumentDirty();
                }

                if (ImGui::Button("Remove Camera Controller"))
                {
                    m_removeComponentRequest = { selectedObject->Id, Pragma::Renderer::ComponentType::CameraController, true };
                }
            }

            if (selectedObject->HasLight() && ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen))
            {
                Pragma::Renderer::LightComponent* light = selectedObject->GetLight();
                std::array<float, 3> lightDirection{ light->Direction[0], light->Direction[1], light->Direction[2] };
                std::array<float, 3> lightColor{ light->Color[0], light->Color[1], light->Color[2] };

                if (ImGui::DragFloat3("Direction", lightDirection.data(), 0.01f, -1.0f, 1.0f))
                {
                    if (ImGui::IsItemActivated())
                    {
                        demoScene.CaptureUndoState("Edit Light");
                    }
                    light->Direction[0] = lightDirection[0];
                    light->Direction[1] = lightDirection[1];
                    light->Direction[2] = lightDirection[2];
                    demoScene.MarkDocumentDirty();
                }

                if (ImGui::ColorEdit3("Color", lightColor.data()))
                {
                    demoScene.CaptureUndoState("Edit Light");
                    light->Color[0] = lightColor[0];
                    light->Color[1] = lightColor[1];
                    light->Color[2] = lightColor[2];
                    demoScene.MarkDocumentDirty();
                }

                if (ImGui::DragFloat("Intensity", &light->Intensity, 0.01f, 0.0f, 100.0f))
                {
                    if (ImGui::IsItemActivated())
                    {
                        demoScene.CaptureUndoState("Edit Light");
                    }
                    demoScene.MarkDocumentDirty();
                }

                if (ImGui::Button("Remove Light"))
                {
                    m_removeComponentRequest = { selectedObject->Id, Pragma::Renderer::ComponentType::Light, true };
                }
            }

            if (selectedObject->HasRigidBody() && ImGui::CollapsingHeader("Rigid Body", ImGuiTreeNodeFlags_DefaultOpen))
            {
                Pragma::Renderer::RigidBodyComponent* rigidBody = selectedObject->GetRigidBody();
                const Pragma::Physics::PhysicsSystem::BodyDebugState bodyDebugState = physicsSystem.GetBodyDebugState(selectedObject->Id);
                if (!selectedObject->HasBoxCollider())
                {
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.35f, 1.0f), "Warning: Rigid Body has no Box Collider.");
                }
                else if (HasInvalidBoxCollider(*selectedObject->GetBoxCollider()))
                {
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.35f, 1.0f), "Warning: Box Collider extent is invalid.");
                }
                else if (rigidBody->Enabled && !bodyDebugState.HasBody)
                {
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.35f, 1.0f), "Warning: runtime body is not created.");
                }
                ImGui::Text("Runtime Body: %s", bodyDebugState.HasBody ? "Created" : "Missing");
                ImGui::Text("Runtime State: %s", bodyDebugState.HasBody ? (bodyDebugState.IsActive ? "Awake" : "Sleeping") : "N/A");
                ImGui::Separator();
                if (ImGui::Checkbox("Enabled", &rigidBody->Enabled))
                {
                    demoScene.CaptureUndoState("Edit Rigid Body");
                    demoScene.MarkDocumentDirty();
                }

                if (ImGui::BeginCombo("Motion Type", ToRigidBodyMotionTypeLabel(rigidBody->MotionType)))
                {
                    constexpr std::array motionTypes
                    {
                        Pragma::Renderer::RigidBodyMotionType::Static,
                        Pragma::Renderer::RigidBodyMotionType::Dynamic,
                        Pragma::Renderer::RigidBodyMotionType::Kinematic
                    };
                    for (const Pragma::Renderer::RigidBodyMotionType motionType : motionTypes)
                    {
                        const bool selected = rigidBody->MotionType == motionType;
                        if (ImGui::Selectable(ToRigidBodyMotionTypeLabel(motionType), selected))
                        {
                            demoScene.CaptureUndoState("Edit Rigid Body");
                            rigidBody->MotionType = motionType;
                            demoScene.MarkDocumentDirty();
                        }
                        if (selected)
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }

                if (ImGui::BeginCombo("Collision Layer", ToRigidBodyCollisionLayerLabel(rigidBody->CollisionLayer)))
                {
                    constexpr std::array collisionLayers
                    {
                        Pragma::Renderer::RigidBodyCollisionLayer::Default,
                        Pragma::Renderer::RigidBodyCollisionLayer::NoCollision
                    };
                    for (const Pragma::Renderer::RigidBodyCollisionLayer collisionLayer : collisionLayers)
                    {
                        const bool selected = rigidBody->CollisionLayer == collisionLayer;
                        if (ImGui::Selectable(ToRigidBodyCollisionLayerLabel(collisionLayer), selected))
                        {
                            demoScene.CaptureUndoState("Edit Rigid Body");
                            rigidBody->CollisionLayer = collisionLayer;
                            demoScene.MarkDocumentDirty();
                        }
                        if (selected)
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }

                if (ImGui::DragFloat("Friction", &rigidBody->Friction, 0.01f, 0.0f, 5.0f))
                {
                    if (ImGui::IsItemActivated())
                    {
                        demoScene.CaptureUndoState("Edit Rigid Body");
                    }
                    demoScene.MarkDocumentDirty();
                }

                if (ImGui::DragFloat("Restitution", &rigidBody->Restitution, 0.01f, 0.0f, 1.0f))
                {
                    if (ImGui::IsItemActivated())
                    {
                        demoScene.CaptureUndoState("Edit Rigid Body");
                    }
                    demoScene.MarkDocumentDirty();
                }

                if (ImGui::DragFloat("Linear Damping", &rigidBody->LinearDamping, 0.01f, 0.0f, 10.0f))
                {
                    if (ImGui::IsItemActivated())
                    {
                        demoScene.CaptureUndoState("Edit Rigid Body");
                    }
                    demoScene.MarkDocumentDirty();
                }

                if (ImGui::DragFloat("Angular Damping", &rigidBody->AngularDamping, 0.01f, 0.0f, 10.0f))
                {
                    if (ImGui::IsItemActivated())
                    {
                        demoScene.CaptureUndoState("Edit Rigid Body");
                    }
                    demoScene.MarkDocumentDirty();
                }

                if (ImGui::DragFloat("Gravity Factor", &rigidBody->GravityFactor, 0.01f, 0.0f, 5.0f))
                {
                    if (ImGui::IsItemActivated())
                    {
                        demoScene.CaptureUndoState("Edit Rigid Body");
                    }
                    demoScene.MarkDocumentDirty();
                }

                if (ImGui::Button("Remove Rigid Body"))
                {
                    m_removeComponentRequest = { selectedObject->Id, Pragma::Renderer::ComponentType::RigidBody, true };
                }
            }

            if (selectedObject->HasBoxCollider() && ImGui::CollapsingHeader("Box Collider", ImGuiTreeNodeFlags_DefaultOpen))
            {
                Pragma::Renderer::BoxColliderComponent* boxCollider = selectedObject->GetBoxCollider();
                if (!selectedObject->HasRigidBody())
                {
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.35f, 1.0f), "Warning: Box Collider has no Rigid Body.");
                }
                if (HasInvalidBoxCollider(*boxCollider))
                {
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.35f, 1.0f), "Warning: half extent must stay above zero.");
                }
                const Pragma::Renderer::Transform worldTransform = scene.GetWorldTransform(selectedObject->Id);
                ImGui::Text(
                    "World Extent: %.2f %.2f %.2f",
                    std::abs(boxCollider->HalfExtent.X * worldTransform.Scale.X) * 2.0f,
                    std::abs(boxCollider->HalfExtent.Y * worldTransform.Scale.Y) * 2.0f,
                    std::abs(boxCollider->HalfExtent.Z * worldTransform.Scale.Z) * 2.0f);
                ImGui::Separator();
                std::array<float, 3> halfExtent
                {
                    boxCollider->HalfExtent.X,
                    boxCollider->HalfExtent.Y,
                    boxCollider->HalfExtent.Z
                };

                if (ImGui::DragFloat3("Half Extent", halfExtent.data(), 0.01f, 0.01f, 100.0f))
                {
                    if (ImGui::IsItemActivated())
                    {
                        demoScene.CaptureUndoState("Edit Box Collider");
                    }
                    boxCollider->HalfExtent = { halfExtent[0], halfExtent[1], halfExtent[2] };
                    demoScene.MarkDocumentDirty();
                }

                if (ImGui::Button("Remove Box Collider"))
                {
                    m_removeComponentRequest = { selectedObject->Id, Pragma::Renderer::ComponentType::BoxCollider, true };
                }
            }

            if (selectedObject->HasMeshRenderer() && ImGui::CollapsingHeader("Mesh Renderer", ImGuiTreeNodeFlags_DefaultOpen))
            {
                const Pragma::Renderer::MeshRendererComponent* meshRenderer = selectedObject->GetMeshRenderer();
                ImGui::Text("Mesh: %s", meshRenderer->Mesh != nullptr ? "Assigned" : "Missing");
                ImGui::Text("Material: %s", meshRenderer->Material != nullptr ? "Assigned" : "Missing");
                ImGui::TextWrapped(
                    "Material Asset: %s",
                    meshRenderer->MaterialAssetId.empty() ? "<None>" : meshRenderer->MaterialAssetId.Value.c_str());
                const std::vector<std::string> materialAssetNames = demoScene.GetAvailableMaterialAssetNames();
                if (!materialAssetNames.empty() && ImGui::BeginCombo("Material Asset", meshRenderer->MaterialAssetId.empty() ? "<None>" : meshRenderer->MaterialAssetId.Value.c_str()))
                {
                    for (const std::string& materialAssetName : materialAssetNames)
                    {
                        const bool selected = materialAssetName == meshRenderer->MaterialAssetId.Value;
                        if (ImGui::Selectable(materialAssetName.c_str(), selected))
                        {
                            m_materialRequest.Entity = selectedObject->Id;
                            m_materialRequest.MaterialAssetName = materialAssetName;
                            m_materialRequest.Pending = true;
                        }
                        if (selected)
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
                ImGui::TextDisabled("Material properties are asset-owned and shared outside the scene file.");

                if (ImGui::Button("Remove Mesh Renderer"))
                {
                    m_removeComponentRequest = { selectedObject->Id, Pragma::Renderer::ComponentType::MeshRenderer, true };
                }
            }

            if (selectedObject->HasPrefabInstance() && ImGui::CollapsingHeader("Prefab Instance", ImGuiTreeNodeFlags_DefaultOpen))
            {
                const auto* prefabInstance = selectedObject->GetPrefabInstance();
                const bool hasOverrides = demoScene.HasPrefabOverrides(selectedObject->Id);
                ImGui::TextWrapped(
                    "Source Asset: %s",
                    prefabInstance != nullptr && !prefabInstance->PrefabAssetId.empty()
                        ? prefabInstance->PrefabAssetId.Value.c_str()
                        : "<None>");
                ImGui::Text("Overrides: %s", hasOverrides ? "Yes" : "No");

                if (prefabInstance != nullptr && !prefabInstance->PrefabAssetId.empty())
                {
                    const std::filesystem::path prefabPath = demoScene.ResolveAssetPath(prefabInstance->PrefabAssetId);
                    ImGui::TextWrapped("Path: %s", prefabPath.string().c_str());
                }

                if (!hasOverrides)
                {
                    ImGui::BeginDisabled();
                }
                if (ImGui::Button("Apply Prefab"))
                {
                    m_prefabApplyRequest.Root = selectedObject->Id;
                    m_prefabApplyRequest.Pending = true;
                }
                if (!hasOverrides)
                {
                    ImGui::EndDisabled();
                }
                ImGui::SameLine();
                if (ImGui::Button("Revert Prefab"))
                {
                    m_prefabRevertRequest.Root = selectedObject->Id;
                    m_prefabRevertRequest.Pending = true;
                }
                if (!hasOverrides)
                {
                    ImGui::TextDisabled("Instance matches source prefab.");
                }
            }

            if (selectedObject->HasManagedScript() && ImGui::CollapsingHeader("Managed Script", ImGuiTreeNodeFlags_DefaultOpen))
            {
                const auto* managedScript = selectedObject->GetManagedScript();
                if (managedScript != nullptr)
                {
                    ImGui::TextWrapped("Project: %s", managedScript->GetProjectAssetId().Value.c_str());
                    ImGui::TextWrapped("Type: %s", managedScript->GetTypeName().c_str());
                    if (!managedScript->GetLastStatus().empty())
                    {
                        ImGui::TextWrapped("Status: %s", managedScript->GetLastStatus().c_str());
                    }

                    if (ImGui::Button("Clear Managed Script"))
                    {
                        m_scriptRequest.Entity = selectedObject->Id;
                        m_scriptRequest.Clear = true;
                        m_scriptRequest.Pending = true;
                    }
                }
            }

            if (selectedObject->HasBehaviour() && !selectedObject->HasManagedScript() &&
                ImGui::CollapsingHeader("Native Script", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto* script = dynamic_cast<Pragma::Renderer::NativeScriptComponent*>(selectedObject->GetBehaviour());
                if (script != nullptr)
                {
                    const std::vector<Pragma::Renderer::NativeScriptMetadata> scripts = demoScene.GetAvailableScripts();
                    const auto currentScriptIt = std::find_if(
                        scripts.begin(),
                        scripts.end(),
                        [&script](const Pragma::Renderer::NativeScriptMetadata& metadata)
                        {
                            return metadata.Id == script->GetScriptName();
                        });
                    const char* currentScriptLabel = currentScriptIt != scripts.end()
                        ? currentScriptIt->DisplayName.c_str()
                        : (script->GetScriptName().empty() ? "<None>" : script->GetScriptName().c_str());
                    ImGui::Text("Current Script: %s", currentScriptLabel);
                    if (currentScriptIt != scripts.end() && !currentScriptIt->Description.empty())
                    {
                        ImGui::TextWrapped("%s", currentScriptIt->Description.c_str());
                    }

                    if (ImGui::BeginCombo("Script Type", currentScriptLabel))
                    {
                        for (const Pragma::Renderer::NativeScriptMetadata& metadata : scripts)
                        {
                            const bool selected = metadata.Id == script->GetScriptName();
                            if (ImGui::Selectable(metadata.DisplayName.c_str(), selected))
                            {
                                m_scriptRequest.Entity = selectedObject->Id;
                                m_scriptRequest.ScriptName = metadata.Id;
                                m_scriptRequest.Clear = false;
                                m_scriptRequest.Pending = true;
                            }
                            if (ImGui::IsItemHovered() && !metadata.Description.empty())
                            {
                                ImGui::SetTooltip("%s\n%s", metadata.Id.c_str(), metadata.Description.c_str());
                            }
                        }
                        ImGui::EndCombo();
                    }

                    if (ImGui::Button("Clear Script"))
                    {
                        m_scriptRequest.Entity = selectedObject->Id;
                        m_scriptRequest.Clear = true;
                        m_scriptRequest.Pending = true;
                    }
                }
            }
            else if (!selectedObject->HasManagedScript())
            {
                const std::vector<Pragma::Renderer::NativeScriptMetadata> scripts = demoScene.GetAvailableScripts();
                if (!scripts.empty() && ImGui::BeginCombo("Add Script", "Select Script"))
                {
                    for (const Pragma::Renderer::NativeScriptMetadata& metadata : scripts)
                    {
                        if (ImGui::Selectable(metadata.DisplayName.c_str(), false))
                        {
                            m_scriptRequest.Entity = selectedObject->Id;
                            m_scriptRequest.ScriptName = metadata.Id;
                            m_scriptRequest.Clear = false;
                            m_scriptRequest.Pending = true;
                        }
                        if (ImGui::IsItemHovered() && !metadata.Description.empty())
                        {
                            ImGui::SetTooltip("%s\n%s", metadata.Id.c_str(), metadata.Description.c_str());
                        }
                    }
                    ImGui::EndCombo();
                }
            }

            if (!selectedObject->HasBehaviour())
            {
                const std::vector<Pragma::Scripting::ManagedScriptTypeMetadata> managedScripts = demoScene.GetAvailableManagedScripts();
                if (!managedScripts.empty() && ImGui::BeginCombo("Add Managed Script", "Select Managed Script"))
                {
                    for (const Pragma::Scripting::ManagedScriptTypeMetadata& metadata : managedScripts)
                    {
                        if (ImGui::Selectable(metadata.DisplayName.c_str(), false))
                        {
                            m_managedScriptRequest.Entity = selectedObject->Id;
                            m_managedScriptRequest.ProjectAssetName = metadata.ProjectAsset.Value;
                            m_managedScriptRequest.TypeName = metadata.TypeName;
                            m_managedScriptRequest.Pending = true;
                        }
                        if (ImGui::IsItemHovered() && !metadata.Description.empty())
                        {
                            ImGui::SetTooltip(
                                "%s\n%s\n%s",
                                metadata.ProjectAsset.Value.c_str(),
                                metadata.TypeName.c_str(),
                                metadata.Description.c_str());
                        }
                    }
                    ImGui::EndCombo();
                }
            }

            if (ImGui::Button("Add Component..."))
            {
                ImGui::OpenPopup("AddComponentPopup");
            }

            if (ImGui::BeginPopup("AddComponentPopup"))
            {
                auto addComponentOption = [&](const Pragma::Renderer::ComponentType type, const bool enabled)
                {
                    if (!enabled)
                    {
                        ImGui::BeginDisabled();
                    }
                    if (ImGui::MenuItem(ToComponentLabel(type)))
                    {
                        m_addComponentRequest.Entity = selectedObject->Id;
                        m_addComponentRequest.Type = type;
                        m_addComponentRequest.Pending = true;
                    }
                    if (!enabled)
                    {
                        ImGui::EndDisabled();
                    }
                };

                addComponentOption(Pragma::Renderer::ComponentType::Camera, !selectedObject->HasCamera());
                addComponentOption(Pragma::Renderer::ComponentType::CameraController, selectedObject->HasCamera() && !selectedObject->HasCameraController());
                addComponentOption(Pragma::Renderer::ComponentType::Light, !selectedObject->HasLight());
                addComponentOption(Pragma::Renderer::ComponentType::MeshRenderer, !selectedObject->HasMeshRenderer());
                addComponentOption(Pragma::Renderer::ComponentType::RigidBody, !selectedObject->HasRigidBody());
                addComponentOption(Pragma::Renderer::ComponentType::BoxCollider, !selectedObject->HasBoxCollider());
                ImGui::EndPopup();
            }
        }
        ImGui::End();
    }

    if (m_showMaterialBrowserWindow)
    {
        ImGui::SetNextWindowSize(ImVec2(420.0f, 420.0f), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Material Browser", &m_showMaterialBrowserWindow))
        {
            const std::vector<std::string> materialAssetNames = demoScene.GetAvailableMaterialAssetNames();
            const std::vector<std::string> textureAssetNames = demoScene.GetAvailableTextureAssetNames();
            ImGui::InputTextWithHint("##MaterialFilter", "Filter materials...", m_materialFilterBuffer.data(), m_materialFilterBuffer.size());
            ImGui::Separator();

            ImGui::BeginChild("MaterialAssetList", ImVec2(0.0f, 220.0f), true);
            for (const std::string& materialAssetName : materialAssetNames)
            {
                if (!MatchesTextFilter(materialAssetName, m_materialFilterBuffer.data()))
                {
                    continue;
                }

                const bool selected = materialAssetName == m_selectedMaterialAssetName;
                if (ImGui::Selectable(materialAssetName.c_str(), selected))
                {
                    m_selectedMaterialAssetName = materialAssetName;
                }
            }
            ImGui::EndChild();

            const bool hasMaterialSelection = !m_selectedMaterialAssetName.empty();
            const Pragma::Renderer::SceneObject* selectedObject = scene.FindObject(m_selectedObjectId);
            const bool canAssignToSelection =
                hasMaterialSelection &&
                selectedObject != nullptr &&
                selectedObject->HasMeshRenderer();

            if (hasMaterialSelection)
            {
                const std::filesystem::path materialPath = demoScene.ResolveAssetPath({ m_selectedMaterialAssetName });
                ImGui::Separator();
                ImGui::TextWrapped("Selected: %s", m_selectedMaterialAssetName.c_str());
                ImGui::TextWrapped("Path: %s", materialPath.string().c_str());

                float baseColor[4]
                {
                    m_materialEditorData.BaseColor[0],
                    m_materialEditorData.BaseColor[1],
                    m_materialEditorData.BaseColor[2],
                    m_materialEditorData.BaseColor[3]
                };
                if (ImGui::ColorEdit4("Base Color", baseColor))
                {
                    for (std::size_t i = 0; i < 4; ++i)
                    {
                        m_materialEditorData.BaseColor[i] = baseColor[i];
                    }
                }

                ImGui::DragFloat("Roughness", &m_materialEditorData.Roughness, 0.01f, 0.0f, 1.0f);
                ImGui::Checkbox("Use Albedo Texture", &m_materialEditorData.UseAlbedoTexture);

                const char* currentTextureLabel = m_materialEditorData.AlbedoTextureAsset.empty()
                    ? "<None>"
                    : m_materialEditorData.AlbedoTextureAsset.Value.c_str();
                if (ImGui::BeginCombo("Albedo Texture", currentTextureLabel))
                {
                    const bool noneSelected = m_materialEditorData.AlbedoTextureAsset.empty();
                    if (ImGui::Selectable("<None>", noneSelected))
                    {
                        m_materialEditorData.AlbedoTextureAsset = {};
                    }
                    if (noneSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }

                    for (const std::string& textureAssetName : textureAssetNames)
                    {
                        const bool selected = textureAssetName == m_materialEditorData.AlbedoTextureAsset.Value;
                        if (ImGui::Selectable(textureAssetName.c_str(), selected))
                        {
                            m_materialEditorData.AlbedoTextureAsset = { textureAssetName };
                        }
                        if (selected)
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
            }
            else
            {
                ImGui::Separator();
                ImGui::TextDisabled("Select a material asset to inspect or assign it.");
            }

            if (!hasMaterialSelection)
            {
                ImGui::BeginDisabled();
            }
            if (ImGui::Button("Save Material Asset"))
            {
                m_materialAssetSaveRequest.MaterialAssetName = m_selectedMaterialAssetName;
                m_materialAssetSaveRequest.MaterialData = m_materialEditorData;
                m_materialAssetSaveRequest.Pending = true;
            }
            if (!hasMaterialSelection)
            {
                ImGui::EndDisabled();
            }

            ImGui::SameLine();
            if (!canAssignToSelection)
            {
                ImGui::BeginDisabled();
            }
            if (ImGui::Button("Assign To Selected"))
            {
                m_materialRequest.Entity = m_selectedObjectId;
                m_materialRequest.MaterialAssetName = m_selectedMaterialAssetName;
                m_materialRequest.Pending = true;
            }
            if (!canAssignToSelection)
            {
                ImGui::EndDisabled();
            }

            if (selectedObject == nullptr)
            {
                ImGui::SameLine();
                ImGui::TextDisabled("No object selected.");
            }
            else if (!selectedObject->HasMeshRenderer())
            {
                ImGui::SameLine();
                ImGui::TextDisabled("Selected object has no Mesh Renderer.");
            }
        }
        ImGui::End();
    }

    if (m_showPrefabBrowserWindow)
    {
        ImGui::SetNextWindowSize(ImVec2(420.0f, 360.0f), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Prefab Browser", &m_showPrefabBrowserWindow))
        {
            const std::vector<std::string> prefabAssetNames = demoScene.GetAvailablePrefabAssetNames();
            const Pragma::Renderer::SceneObject* selectedObject = scene.FindObject(m_selectedObjectId);
            ImGui::InputTextWithHint("##PrefabFilter", "Filter prefabs...", m_prefabFilterBuffer.data(), m_prefabFilterBuffer.size());
            ImGui::Separator();

            ImGui::BeginChild("PrefabAssetList", ImVec2(0.0f, 180.0f), true);
            for (const std::string& prefabAssetName : prefabAssetNames)
            {
                if (!MatchesTextFilter(prefabAssetName, m_prefabFilterBuffer.data()))
                {
                    continue;
                }

                const bool selected = prefabAssetName == m_selectedPrefabAssetName;
                if (ImGui::Selectable(prefabAssetName.c_str(), selected))
                {
                    m_selectedPrefabAssetName = prefabAssetName;
                }
            }
            ImGui::EndChild();

            const bool hasPrefabSelection = !m_selectedPrefabAssetName.empty();
            if (hasPrefabSelection)
            {
                const std::filesystem::path prefabPath = demoScene.ResolveAssetPath({ m_selectedPrefabAssetName });
                ImGui::Separator();
                ImGui::TextWrapped("Selected: %s", m_selectedPrefabAssetName.c_str());
                ImGui::TextWrapped("Path: %s", prefabPath.string().c_str());
            }
            else
            {
                ImGui::Separator();
                ImGui::TextDisabled("Select a prefab asset to instantiate it into the scene.");
            }

            const bool canInstantiateAsChild =
                hasPrefabSelection &&
                m_selectedObjectId != Pragma::Renderer::InvalidEntityId &&
                scene.IsEntityAlive(m_selectedObjectId);

            if (!hasPrefabSelection)
            {
                ImGui::BeginDisabled();
            }
            if (ImGui::Button("Instantiate"))
            {
                m_prefabInstantiateRequest.PrefabAssetName = m_selectedPrefabAssetName;
                m_prefabInstantiateRequest.Parent = Pragma::Renderer::InvalidEntityId;
                m_prefabInstantiateRequest.Pending = true;
            }
            if (!hasPrefabSelection)
            {
                ImGui::EndDisabled();
            }

            ImGui::SameLine();
            if (!canInstantiateAsChild)
            {
                ImGui::BeginDisabled();
            }
            if (ImGui::Button("Instantiate As Child"))
            {
                m_prefabInstantiateRequest.PrefabAssetName = m_selectedPrefabAssetName;
                m_prefabInstantiateRequest.Parent = m_selectedObjectId;
                m_prefabInstantiateRequest.Pending = true;
            }
            if (!canInstantiateAsChild)
            {
                ImGui::EndDisabled();
            }

            if (!canInstantiateAsChild)
            {
                ImGui::SameLine();
                ImGui::TextDisabled("Select a parent object to instantiate as child.");
            }

            ImGui::Separator();
            const bool canSavePrefab = selectedObject != nullptr;
            if (!canSavePrefab)
            {
                ImGui::BeginDisabled();
            }
            if (ImGui::Button("Save Selected As Prefab"))
            {
                CopyTextToBuffer(
                    m_prefabSaveNameBuffer,
                    selectedObject != nullptr && selectedObject->HasPrefabInstance() && !selectedObject->GetPrefabInstance()->PrefabAssetId.empty()
                        ? selectedObject->GetPrefabInstance()->PrefabAssetId.Value
                        : MakePrefabAssetNameFromObjectName(selectedObject != nullptr ? selectedObject->Name : std::string{}));
                ImGui::OpenPopup("Save Prefab");
            }
            if (!canSavePrefab)
            {
                ImGui::EndDisabled();
            }

            if (ImGui::BeginPopupModal("Save Prefab", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::InputText("Prefab Asset Id", m_prefabSaveNameBuffer.data(), m_prefabSaveNameBuffer.size());
                ImGui::TextWrapped(
                    "This saves the selected object and its full subtree as a prefab asset in the project manifest.");

                if (ImGui::Button("Save"))
                {
                    m_prefabSaveRequest.PrefabAssetName = m_prefabSaveNameBuffer.data();
                    m_prefabSaveRequest.Root = m_selectedObjectId;
                    m_prefabSaveRequest.Pending = true;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel"))
                {
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }
        }
        ImGui::End();
    }

    if (m_showNotificationsWindow)
    {
        ImGui::SetNextWindowSize(ImVec2(500.0f, 72.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(16.0f, 40.0f), ImGuiCond_FirstUseEver);
        const ImGuiWindowFlags notificationFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
        if (ImGui::Begin("Editor Notifications", &m_showNotificationsWindow, notificationFlags))
        {
            if (m_notifications.empty())
            {
                ImGui::TextDisabled("No recent editor notifications.");
            }
            else
            {
                for (const Notification& notification : m_notifications)
                {
                    ImGui::BulletText("%s", notification.Message.c_str());
                }
            }
            ImGui::End();
        }
    }

    if (m_showPhysicsDebugWindow)
    {
        ImGui::SetNextWindowSize(ImVec2(420.0f, 260.0f), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Physics Debug", &m_showPhysicsDebugWindow))
        {
            const std::size_t bodyCount = physicsSystem.GetBodyCount();
            const std::size_t activeBodyCount = physicsSystem.GetActiveBodyCount();
            ImGui::Text("Bodies: %llu", static_cast<unsigned long long>(bodyCount));
            ImGui::Text("Active Bodies: %llu", static_cast<unsigned long long>(activeBodyCount));
            ImGui::Separator();

            bool anyPhysicsObject = false;
            for (const Pragma::Renderer::SceneObject& object : scene.GetObjects())
            {
                if (!object.HasRigidBody() && !object.HasBoxCollider())
                {
                    continue;
                }

                anyPhysicsObject = true;
                const Pragma::Physics::PhysicsSystem::BodyDebugState bodyDebugState = physicsSystem.GetBodyDebugState(object.Id);
                const bool incompletePhysicsSetup = object.HasRigidBody() != object.HasBoxCollider();
                const bool invalidCollider = object.HasBoxCollider() && HasInvalidBoxCollider(*object.GetBoxCollider());
                const bool selected = m_selectedObjectId == object.Id;
                if (ImGui::Selectable(object.Name.c_str(), selected))
                {
                    m_selectedObjectId = object.Id;
                }

                ImGui::Indent();
                ImGui::Text("EntityId: %llu", static_cast<unsigned long long>(object.Id));
                ImGui::Text("Runtime Body: %s", bodyDebugState.HasBody ? "Created" : "Missing");
                ImGui::Text("Runtime State: %s", bodyDebugState.HasBody ? (bodyDebugState.IsActive ? "Awake" : "Sleeping") : "N/A");
                if (incompletePhysicsSetup)
                {
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.35f, 1.0f), "Issue: incomplete physics setup");
                }
                else if (invalidCollider)
                {
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.35f, 1.0f), "Issue: invalid collider extent");
                }
                if (const Pragma::Renderer::RigidBodyComponent* rigidBody = object.GetRigidBody(); rigidBody != nullptr)
                {
                    ImGui::Text("Motion: %s", ToRigidBodyMotionTypeLabel(rigidBody->MotionType));
                    ImGui::Text("Layer: %s", ToRigidBodyCollisionLayerLabel(rigidBody->CollisionLayer));
                    ImGui::Text("Enabled: %s", rigidBody->Enabled ? "Yes" : "No");
                }
                if (const Pragma::Renderer::BoxColliderComponent* boxCollider = object.GetBoxCollider(); boxCollider != nullptr)
                {
                    ImGui::Text(
                        "Half Extent: %.2f %.2f %.2f",
                        boxCollider->HalfExtent.X,
                        boxCollider->HalfExtent.Y,
                        boxCollider->HalfExtent.Z);
                }
                ImGui::Unindent();
                ImGui::Separator();
            }

            if (!anyPhysicsObject)
            {
                ImGui::TextDisabled("No physics-enabled objects in the current scene.");
            }
            ImGui::End();
        }
    }

    if (m_showStatusWindow)
    {
        ImGui::SetNextWindowSize(ImVec2(520.0f, 64.0f), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Status", &m_showStatusWindow))
        {
            const Pragma::Renderer::SceneObject* selectedObject = scene.FindObject(m_selectedObjectId);
            ImGui::Text("Selection: %s", selectedObject != nullptr ? selectedObject->Name.c_str() : "None");
            ImGui::Text("Tool: %s", m_gizmoMode == GizmoMode::Move ? "Move" : (m_gizmoMode == GizmoMode::Rotate ? "Rotate" : "Scale"));
            ImGui::Text("Space: %s", m_useLocalGizmoSpace ? "Local" : "World");
            ImGui::End();
        }
    }
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z, false))
    {
        m_undoRequested = true;
    }
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y, false))
    {
        m_redoRequested = true;
    }

    if (m_resetLayoutRequested)
    {
        ResetLayoutState(showDiagnosticsWindow, showProfilerWindow, showLogConsoleWindow);
        m_resetLayoutRequested = false;
        PushNotification("Editor layout reset.");
    }

    const LayoutState currentLayoutState = CaptureLayoutState(showDiagnosticsWindow, showProfilerWindow, showLogConsoleWindow);
    if (currentLayoutState != m_lastSavedLayoutState)
    {
        SaveLayoutState(currentLayoutState);
        m_lastSavedLayoutState = currentLayoutState;
    }

    if (io.IniFilename != nullptr && !std::filesystem::exists(io.IniFilename))
    {
        ImGui::SaveIniSettingsToDisk(io.IniFilename);
    }
}

void EditorUI::ApplyPendingSceneActions(Pragma::Core::DemoScene& demoScene)
{
    if (m_undoRequested)
    {
        m_undoRequested = false;
        if (demoScene.UndoDocument())
        {
            PushNotification("Undo: " + demoScene.GetLastHistoryAction());
        }
    }

    if (m_redoRequested)
    {
        m_redoRequested = false;
        if (demoScene.RedoDocument())
        {
            PushNotification("Redo: " + demoScene.GetLastHistoryAction());
        }
    }

    if (m_createObjectRequest.Pending)
    {
        m_createObjectRequest.Pending = false;
        const Pragma::Renderer::EntityId createdId = demoScene.CreateObject(
            m_createObjectRequest.Name,
            m_createObjectRequest.ObjectTemplate,
            m_createObjectRequest.Parent);
        if (createdId != Pragma::Renderer::InvalidEntityId)
        {
            m_selectedObjectId = createdId;
            PushNotification(
                "Created " + std::string(ToObjectTemplateLabel(m_createObjectRequest.ObjectTemplate)) +
                " '" + m_createObjectRequest.Name + "'.");
        }
        else
        {
            if (m_createObjectRequest.ObjectTemplate == Pragma::Core::SceneObjectTemplate::DirectionalLight)
            {
                PushNotification("Create failed: scene already has a directional light.");
            }
            else
            {
                PushNotification("Create object failed.");
            }
        }
        m_createObjectRequest = {};
        m_createObjectParentId = Pragma::Renderer::InvalidEntityId;
        m_createObjectTemplate = Pragma::Core::SceneObjectTemplate::Empty;
    }

    if (m_duplicateObjectId != Pragma::Renderer::InvalidEntityId)
    {
        const Pragma::Renderer::EntityId sourceId = m_duplicateObjectId;
        m_duplicateObjectId = Pragma::Renderer::InvalidEntityId;
        const Pragma::Renderer::SceneObject* sourceObject = demoScene.GetScene().FindObject(sourceId);
        const bool subtreeContainsLight = SubtreeContainsDirectionalLight(demoScene.GetScene(), sourceId);
        const Pragma::Renderer::EntityId duplicateId = demoScene.DuplicateObject(sourceId);
        if (duplicateId != Pragma::Renderer::InvalidEntityId)
        {
            m_selectedObjectId = duplicateId;
            PushNotification("Duplicated '" + (sourceObject != nullptr ? sourceObject->Name : std::string("object")) + "'.");
        }
        else if (subtreeContainsLight)
        {
            PushNotification("Duplicate failed: scene already supports one directional light.");
        }
        else
        {
            PushNotification("Duplicate failed.");
        }
    }

    if (m_deleteObjectId != Pragma::Renderer::InvalidEntityId)
    {
        const Pragma::Renderer::EntityId deleteId = m_deleteObjectId;
        m_deleteObjectId = Pragma::Renderer::InvalidEntityId;
        if (demoScene.DeleteObject(deleteId))
        {
            if (m_selectedObjectId == deleteId)
            {
                m_selectedObjectId = Pragma::Renderer::InvalidEntityId;
            }
            PushNotification("Deleted object.");
        }
    }

    if (m_parentRequest.Pending)
    {
        m_parentRequest.Pending = false;
        if (demoScene.SetParent(m_parentRequest.Child, m_parentRequest.Parent))
        {
            PushNotification(m_parentRequest.Parent == Pragma::Renderer::InvalidEntityId ? "Object unparented." : "Object reparented.");
        }
        m_parentRequest = {};
    }

    if (m_addComponentRequest.Pending)
    {
        m_addComponentRequest.Pending = false;
        if (demoScene.AddComponent(m_addComponentRequest.Entity, m_addComponentRequest.Type))
        {
            PushNotification(std::string("Added component: ") + ToComponentLabel(m_addComponentRequest.Type));
        }
        m_addComponentRequest = {};
    }

    if (m_removeComponentRequest.Pending)
    {
        m_removeComponentRequest.Pending = false;
        if (demoScene.RemoveComponent(m_removeComponentRequest.Entity, m_removeComponentRequest.Type))
        {
            PushNotification(std::string("Removed component: ") + ToComponentLabel(m_removeComponentRequest.Type));
        }
        m_removeComponentRequest = {};
    }

    if (m_scriptRequest.Pending)
    {
        m_scriptRequest.Pending = false;
        if (m_scriptRequest.Clear)
        {
            if (demoScene.ClearScript(m_scriptRequest.Entity))
            {
                PushNotification("Script cleared.");
            }
        }
        else if (demoScene.SetScript(m_scriptRequest.Entity, m_scriptRequest.ScriptName))
        {
            PushNotification("Assigned script '" + m_scriptRequest.ScriptName + "'.");
        }
        m_scriptRequest = {};
    }

    if (m_managedScriptRequest.Pending)
    {
        m_managedScriptRequest.Pending = false;
        if (demoScene.SetManagedScript(
            m_managedScriptRequest.Entity,
            { m_managedScriptRequest.ProjectAssetName },
            m_managedScriptRequest.TypeName))
        {
            PushNotification("Assigned managed script '" + m_managedScriptRequest.TypeName + "'.");
        }
        m_managedScriptRequest = {};
    }

    if (m_materialRequest.Pending)
    {
        m_materialRequest.Pending = false;
        if (demoScene.SetMaterialAsset(m_materialRequest.Entity, { m_materialRequest.MaterialAssetName }))
        {
            PushNotification("Assigned material '" + m_materialRequest.MaterialAssetName + "'.");
        }
        m_materialRequest = {};
    }

    if (m_materialAssetSaveRequest.Pending)
    {
        m_materialAssetSaveRequest.Pending = false;
        if (demoScene.SaveMaterialAssetData(
            { m_materialAssetSaveRequest.MaterialAssetName },
            m_materialAssetSaveRequest.MaterialData))
        {
            PushNotification("Saved material '" + m_materialAssetSaveRequest.MaterialAssetName + "'.");
        }
        m_materialAssetSaveRequest = {};
    }

    if (m_prefabInstantiateRequest.Pending)
    {
        m_prefabInstantiateRequest.Pending = false;
        const Pragma::Renderer::EntityId instantiatedRootId = demoScene.InstantiatePrefab(
            { m_prefabInstantiateRequest.PrefabAssetName },
            m_prefabInstantiateRequest.Parent);
        if (instantiatedRootId != Pragma::Renderer::InvalidEntityId)
        {
            m_selectedObjectId = instantiatedRootId;
            PushNotification("Instantiated prefab '" + m_prefabInstantiateRequest.PrefabAssetName + "'.");
        }
        else
        {
            PushNotification("Prefab instantiation failed.");
        }
        m_prefabInstantiateRequest = {};
    }

    if (m_prefabSaveRequest.Pending)
    {
        m_prefabSaveRequest.Pending = false;
        if (demoScene.SavePrefab(m_prefabSaveRequest.Root, { m_prefabSaveRequest.PrefabAssetName }))
        {
            m_selectedPrefabAssetName = m_prefabSaveRequest.PrefabAssetName;
            PushNotification("Saved prefab '" + m_prefabSaveRequest.PrefabAssetName + "'.");
        }
        else
        {
            PushNotification("Save prefab failed.");
        }
        m_prefabSaveRequest = {};
    }

    if (m_prefabApplyRequest.Pending)
    {
        m_prefabApplyRequest.Pending = false;
        const Pragma::Renderer::SceneObject* prefabRoot = demoScene.GetScene().FindObject(m_prefabApplyRequest.Root);
        const auto* prefabInstance = prefabRoot != nullptr ? prefabRoot->GetPrefabInstance() : nullptr;
        const std::string prefabAssetName =
            prefabInstance != nullptr && !prefabInstance->PrefabAssetId.empty()
                ? prefabInstance->PrefabAssetId.Value
                : std::string("prefab");
        if (demoScene.ApplyPrefab(m_prefabApplyRequest.Root))
        {
            PushNotification("Applied '" + prefabAssetName + "'.");
        }
        else
        {
            PushNotification("Apply prefab failed for '" + prefabAssetName + "'.");
        }
        m_prefabApplyRequest = {};
    }

    if (m_prefabRevertRequest.Pending)
    {
        m_prefabRevertRequest.Pending = false;
        const Pragma::Renderer::SceneObject* prefabRoot = demoScene.GetScene().FindObject(m_prefabRevertRequest.Root);
        const auto* prefabInstance = prefabRoot != nullptr ? prefabRoot->GetPrefabInstance() : nullptr;
        const std::string prefabAssetName =
            prefabInstance != nullptr && !prefabInstance->PrefabAssetId.empty()
                ? prefabInstance->PrefabAssetId.Value
                : std::string("prefab");
        const Pragma::Renderer::EntityId newRootId = demoScene.RevertPrefab(m_prefabRevertRequest.Root);
        if (newRootId != Pragma::Renderer::InvalidEntityId)
        {
            m_selectedObjectId = newRootId;
            PushNotification("Reverted '" + prefabAssetName + "'.");
        }
        else
        {
            PushNotification("Revert prefab failed for '" + prefabAssetName + "'.");
        }
        m_prefabRevertRequest = {};
    }

    if (m_saveSceneRequested)
    {
        m_saveSceneRequested = false;
        demoScene.SaveDocument();
        PushNotification("Scene saved.");
    }

    if (m_reloadSceneRequested)
    {
        m_reloadSceneRequested = false;
        demoScene.ReloadDocument();
        if (!demoScene.GetScene().IsEntityAlive(m_selectedObjectId))
        {
            m_selectedObjectId = Pragma::Renderer::InvalidEntityId;
        }
        PushNotification("Scene reloaded.");
    }
}

Pragma::Renderer::EntityId EditorUI::GetSelectedObjectId() const noexcept
{
    return m_selectedObjectId;
}

bool EditorUI::IsPhysicsOverlayEnabled() const noexcept
{
    return m_showPhysicsOverlay;
}

void EditorUI::SyncObjectNameEditor(const Pragma::Renderer::SceneObject& object)
{
    if (m_objectNameBufferEntityId == object.Id)
    {
        return;
    }

    CopyTextToBuffer(m_objectNameBuffer, object.Name);
    m_objectNameBufferEntityId = object.Id;
}

void EditorUI::LoadLayoutState(
    bool& showDiagnosticsWindow,
    bool& showProfilerWindow,
    bool& showLogConsoleWindow)
{
    m_layoutStatePath = GetEditorSavedDirectory() / "editor_layout_state.ini";
    m_layoutLoaded = true;

    LayoutState state;
    std::ifstream input(m_layoutStatePath);
    if (input.is_open())
    {
        std::string line;
        while (std::getline(input, line))
        {
            const std::size_t separatorIndex = line.find('=');
            if (separatorIndex == std::string::npos)
            {
                continue;
            }

            const std::string key = line.substr(0, separatorIndex);
            const bool value = ParseBoolValue(line.substr(separatorIndex + 1));

            if (key == "show_editor")
            {
                state.ShowEditorWindow = value;
            }
            else if (key == "show_hierarchy")
            {
                state.ShowHierarchyWindow = value;
            }
            else if (key == "show_scene_view")
            {
                state.ShowSceneViewWindow = value;
            }
            else if (key == "show_inspector")
            {
                state.ShowInspectorWindow = value;
            }
            else if (key == "show_material_browser")
            {
                state.ShowMaterialBrowserWindow = value;
            }
            else if (key == "show_prefab_browser")
            {
                state.ShowPrefabBrowserWindow = value;
            }
            else if (key == "show_physics_debug")
            {
                state.ShowPhysicsDebugWindow = value;
            }
            else if (key == "show_physics_overlay")
            {
                state.ShowPhysicsOverlay = value;
            }
            else if (key == "show_notifications")
            {
                state.ShowNotificationsWindow = value;
            }
            else if (key == "show_status")
            {
                state.ShowStatusWindow = value;
            }
            else if (key == "show_diagnostics")
            {
                state.ShowDiagnosticsWindow = value;
            }
            else if (key == "show_profiler")
            {
                state.ShowProfilerWindow = value;
            }
            else if (key == "show_log_console")
            {
                state.ShowLogConsoleWindow = value;
            }
        }
    }

    m_showEditorWindow = state.ShowEditorWindow;
    m_showHierarchyWindow = state.ShowHierarchyWindow;
    m_showSceneViewWindow = state.ShowSceneViewWindow;
    m_showInspectorWindow = state.ShowInspectorWindow;
    m_showMaterialBrowserWindow = state.ShowMaterialBrowserWindow;
    m_showPrefabBrowserWindow = state.ShowPrefabBrowserWindow;
    m_showPhysicsDebugWindow = state.ShowPhysicsDebugWindow;
    m_showPhysicsOverlay = state.ShowPhysicsOverlay;
    m_showNotificationsWindow = state.ShowNotificationsWindow;
    m_showStatusWindow = state.ShowStatusWindow;
    showDiagnosticsWindow = state.ShowDiagnosticsWindow;
    showProfilerWindow = state.ShowProfilerWindow;
    showLogConsoleWindow = state.ShowLogConsoleWindow;
    m_lastSavedLayoutState = state;

    if (!std::filesystem::exists(m_layoutStatePath))
    {
        SaveLayoutState(state);
    }
}

void EditorUI::SaveLayoutState(const LayoutState& state) const
{
    const std::filesystem::path path = m_layoutStatePath.empty()
        ? GetEditorSavedDirectory() / "editor_layout_state.ini"
        : m_layoutStatePath;

    std::ofstream output(path, std::ios::trunc);
    if (!output.is_open())
    {
        return;
    }

    auto writeBool = [&output](const char* key, const bool value)
    {
        output << key << '=' << (value ? "true" : "false") << '\n';
    };

    writeBool("show_editor", state.ShowEditorWindow);
    writeBool("show_hierarchy", state.ShowHierarchyWindow);
    writeBool("show_scene_view", state.ShowSceneViewWindow);
    writeBool("show_inspector", state.ShowInspectorWindow);
    writeBool("show_material_browser", state.ShowMaterialBrowserWindow);
    writeBool("show_prefab_browser", state.ShowPrefabBrowserWindow);
    writeBool("show_physics_debug", state.ShowPhysicsDebugWindow);
    writeBool("show_physics_overlay", state.ShowPhysicsOverlay);
    writeBool("show_notifications", state.ShowNotificationsWindow);
    writeBool("show_status", state.ShowStatusWindow);
    writeBool("show_diagnostics", state.ShowDiagnosticsWindow);
    writeBool("show_profiler", state.ShowProfilerWindow);
    writeBool("show_log_console", state.ShowLogConsoleWindow);
}

EditorUI::LayoutState EditorUI::CaptureLayoutState(
    const bool showDiagnosticsWindow,
    const bool showProfilerWindow,
    const bool showLogConsoleWindow) const noexcept
{
    LayoutState state;
    state.ShowEditorWindow = m_showEditorWindow;
    state.ShowHierarchyWindow = m_showHierarchyWindow;
    state.ShowSceneViewWindow = m_showSceneViewWindow;
    state.ShowInspectorWindow = m_showInspectorWindow;
    state.ShowMaterialBrowserWindow = m_showMaterialBrowserWindow;
    state.ShowPrefabBrowserWindow = m_showPrefabBrowserWindow;
    state.ShowPhysicsDebugWindow = m_showPhysicsDebugWindow;
    state.ShowPhysicsOverlay = m_showPhysicsOverlay;
    state.ShowNotificationsWindow = m_showNotificationsWindow;
    state.ShowStatusWindow = m_showStatusWindow;
    state.ShowDiagnosticsWindow = showDiagnosticsWindow;
    state.ShowProfilerWindow = showProfilerWindow;
    state.ShowLogConsoleWindow = showLogConsoleWindow;
    return state;
}

void EditorUI::ResetLayoutState(
    bool& showDiagnosticsWindow,
    bool& showProfilerWindow,
    bool& showLogConsoleWindow)
{
    m_showEditorWindow = true;
    m_showHierarchyWindow = true;
    m_showSceneViewWindow = true;
    m_showInspectorWindow = true;
    m_showMaterialBrowserWindow = true;
    m_showPrefabBrowserWindow = true;
    m_showPhysicsDebugWindow = true;
    m_showPhysicsOverlay = false;
    m_showNotificationsWindow = true;
    m_showStatusWindow = true;
    showDiagnosticsWindow = true;
    showProfilerWindow = true;
    showLogConsoleWindow = true;

    ImGui::LoadIniSettingsFromMemory("");
    if (ImGui::GetIO().IniFilename != nullptr)
    {
        ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);
    }
}

void EditorUI::OpenCreateObjectDialog(const Pragma::Core::SceneObjectTemplate objectTemplate, const Pragma::Renderer::EntityId parentId)
{
    m_createObjectTemplate = objectTemplate;
    m_createObjectParentId = parentId;
    CopyTextToBuffer(m_createObjectNameBuffer, GetDefaultObjectName(objectTemplate));
    m_focusCreateObjectName = true;
    ImGui::OpenPopup("Create Object");
}

void EditorUI::PushNotification(std::string message, const float lifetimeSeconds)
{
    Notification notification;
    notification.Message = std::move(message);
    notification.RemainingSeconds = lifetimeSeconds;
    m_notifications.push_back(std::move(notification));
    if (m_notifications.size() > 8)
    {
        m_notifications.erase(m_notifications.begin());
    }
}
}
