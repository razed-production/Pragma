#include "Pragma/Core/SceneStateBridge.h"

#include "Pragma/Renderer/Material.h"
#include "Pragma/Renderer/ManagedScriptComponent.h"
#include "Pragma/Renderer/MeshRendererComponent.h"
#include "Pragma/Renderer/NativeScriptComponent.h"
#include "Pragma/Renderer/PrefabInstanceComponent.h"

#include <algorithm>
#include <stdexcept>

namespace Pragma::Core
{
namespace
{
bool AreAssetIdsEqual(const Pragma::Assets::AssetId& lhs, const Pragma::Assets::AssetId& rhs) noexcept
{
    return lhs.Value == rhs.Value;
}

bool AreTransformsEqual(const Pragma::Renderer::Transform& lhs, const Pragma::Renderer::Transform& rhs) noexcept
{
    return lhs.Position.X == rhs.Position.X &&
        lhs.Position.Y == rhs.Position.Y &&
        lhs.Position.Z == rhs.Position.Z &&
        lhs.RotationRadians.X == rhs.RotationRadians.X &&
        lhs.RotationRadians.Y == rhs.RotationRadians.Y &&
        lhs.RotationRadians.Z == rhs.RotationRadians.Z &&
        lhs.Scale.X == rhs.Scale.X &&
        lhs.Scale.Y == rhs.Scale.Y &&
        lhs.Scale.Z == rhs.Scale.Z;
}

bool AreMeshRenderersEqual(const SerializedMeshRenderer& lhs, const SerializedMeshRenderer& rhs) noexcept
{
    return AreAssetIdsEqual(lhs.MeshAsset, rhs.MeshAsset) &&
        AreAssetIdsEqual(lhs.MediumLodMeshAsset, rhs.MediumLodMeshAsset) &&
        AreAssetIdsEqual(lhs.LowLodMeshAsset, rhs.LowLodMeshAsset) &&
        AreAssetIdsEqual(lhs.MaterialAsset, rhs.MaterialAsset);
}

bool AreCameraControllersEqual(
    const Pragma::Renderer::CameraControllerComponent& lhs,
    const Pragma::Renderer::CameraControllerComponent& rhs) noexcept
{
    return lhs.MoveSpeed == rhs.MoveSpeed &&
        lhs.FastMoveSpeed == rhs.FastMoveSpeed &&
        lhs.KeyboardLookSpeed == rhs.KeyboardLookSpeed &&
        lhs.MouseLookSensitivity == rhs.MouseLookSensitivity &&
        lhs.Enabled == rhs.Enabled;
}

bool AreLightsEqual(const Pragma::Renderer::LightComponent& lhs, const Pragma::Renderer::LightComponent& rhs) noexcept
{
    return lhs.Direction[0] == rhs.Direction[0] &&
        lhs.Direction[1] == rhs.Direction[1] &&
        lhs.Direction[2] == rhs.Direction[2] &&
        lhs.Intensity == rhs.Intensity &&
        lhs.Color[0] == rhs.Color[0] &&
        lhs.Color[1] == rhs.Color[1] &&
        lhs.Color[2] == rhs.Color[2];
}

bool AreRigidBodiesEqual(const Pragma::Renderer::RigidBodyComponent& lhs, const Pragma::Renderer::RigidBodyComponent& rhs) noexcept
{
    return lhs.Enabled == rhs.Enabled &&
        lhs.MotionType == rhs.MotionType &&
        lhs.CollisionLayer == rhs.CollisionLayer &&
        lhs.Friction == rhs.Friction &&
        lhs.Restitution == rhs.Restitution &&
        lhs.LinearDamping == rhs.LinearDamping &&
        lhs.AngularDamping == rhs.AngularDamping &&
        lhs.GravityFactor == rhs.GravityFactor;
}

bool AreBoxCollidersEqual(const Pragma::Renderer::BoxColliderComponent& lhs, const Pragma::Renderer::BoxColliderComponent& rhs) noexcept
{
    return lhs.HalfExtent.X == rhs.HalfExtent.X &&
        lhs.HalfExtent.Y == rhs.HalfExtent.Y &&
        lhs.HalfExtent.Z == rhs.HalfExtent.Z;
}

bool AreSceneObjectsEqual(const SerializedSceneObject& lhs, const SerializedSceneObject& rhs) noexcept
{
    const bool camerasEqual = lhs.Camera.has_value() == rhs.Camera.has_value() &&
        (!lhs.Camera.has_value() || (
            lhs.Camera->PitchRadians == rhs.Camera->PitchRadians &&
            lhs.Camera->FieldOfViewRadians == rhs.Camera->FieldOfViewRadians &&
            lhs.Camera->NearPlane == rhs.Camera->NearPlane &&
            lhs.Camera->FarPlane == rhs.Camera->FarPlane));
    const bool cameraControllersEqual = lhs.CameraController.has_value() == rhs.CameraController.has_value() &&
        (!lhs.CameraController.has_value() || AreCameraControllersEqual(*lhs.CameraController, *rhs.CameraController));
    const bool meshRenderersEqual = lhs.MeshRenderer.has_value() == rhs.MeshRenderer.has_value() &&
        (!lhs.MeshRenderer.has_value() || AreMeshRenderersEqual(*lhs.MeshRenderer, *rhs.MeshRenderer));
    const bool lightsEqual = lhs.Light.has_value() == rhs.Light.has_value() &&
        (!lhs.Light.has_value() || AreLightsEqual(*lhs.Light, *rhs.Light));
    const bool rigidBodiesEqual = lhs.RigidBody.has_value() == rhs.RigidBody.has_value() &&
        (!lhs.RigidBody.has_value() || AreRigidBodiesEqual(*lhs.RigidBody, *rhs.RigidBody));
    const bool boxCollidersEqual = lhs.BoxCollider.has_value() == rhs.BoxCollider.has_value() &&
        (!lhs.BoxCollider.has_value() || AreBoxCollidersEqual(*lhs.BoxCollider, *rhs.BoxCollider));

    return lhs.Id == rhs.Id &&
        lhs.ParentId == rhs.ParentId &&
        lhs.Name == rhs.Name &&
        AreAssetIdsEqual(lhs.PrefabAssetId, rhs.PrefabAssetId) &&
        AreTransformsEqual(lhs.Transform, rhs.Transform) &&
        meshRenderersEqual &&
        camerasEqual &&
        cameraControllersEqual &&
        lightsEqual &&
        rigidBodiesEqual &&
        boxCollidersEqual &&
        lhs.ScriptName == rhs.ScriptName &&
        AreAssetIdsEqual(lhs.ManagedScriptProjectAsset, rhs.ManagedScriptProjectAsset) &&
        lhs.ManagedScriptTypeName == rhs.ManagedScriptTypeName &&
        lhs.IsActiveCamera == rhs.IsActiveCamera;
}
}

SerializedScene SceneStateBridge::Capture(const Pragma::Renderer::Scene& scene, const SerializedScene& serializedScene)
{
    SerializedScene snapshot = serializedScene;
    const auto& objects = scene.GetObjects();
    for (const Pragma::Renderer::SceneObject& runtimeObject : objects)
    {
        auto serializedIt = std::find_if(
            snapshot.Objects.begin(),
            snapshot.Objects.end(),
            [&runtimeObject](const SerializedSceneObject& object)
            {
                return object.Id == runtimeObject.Id;
            });
        if (serializedIt == snapshot.Objects.end())
        {
            throw std::runtime_error("SceneStateBridge cannot capture state because a runtime object is missing from the serialized scene.");
        }

        serializedIt->Name = runtimeObject.Name;
        serializedIt->ParentId = runtimeObject.ParentId;
        serializedIt->PrefabAssetId = {};
        serializedIt->Transform = runtimeObject.GetTransform();
        serializedIt->IsActiveCamera = runtimeObject.Id == scene.GetActiveCameraEntityId();

        if (const auto* prefabInstance = runtimeObject.GetPrefabInstance(); prefabInstance != nullptr)
        {
            serializedIt->PrefabAssetId = prefabInstance->PrefabAssetId;
        }

        if (const auto* camera = runtimeObject.GetCamera(); camera != nullptr)
        {
            serializedIt->Camera = *camera;
        }
        else
        {
            serializedIt->Camera.reset();
        }

        if (const auto* cameraController = runtimeObject.GetCameraController(); cameraController != nullptr)
        {
            serializedIt->CameraController = *cameraController;
        }
        else
        {
            serializedIt->CameraController.reset();
        }

        if (const auto* meshRenderer = runtimeObject.GetMeshRenderer();
            meshRenderer != nullptr && serializedIt->MeshRenderer.has_value())
        {
            serializedIt->MeshRenderer->MeshAsset = meshRenderer->MeshAssetId;
            serializedIt->MeshRenderer->MediumLodMeshAsset = meshRenderer->MediumLodMeshAssetId;
            serializedIt->MeshRenderer->LowLodMeshAsset = meshRenderer->LowLodMeshAssetId;
            serializedIt->MeshRenderer->MaterialAsset = meshRenderer->MaterialAssetId;
        }
        else if (runtimeObject.GetMeshRenderer() == nullptr)
        {
            serializedIt->MeshRenderer.reset();
        }

        if (const auto* nativeScript = dynamic_cast<const Pragma::Renderer::NativeScriptComponent*>(runtimeObject.GetBehaviour());
            nativeScript != nullptr)
        {
            serializedIt->ScriptName = nativeScript->GetScriptName();
            serializedIt->ManagedScriptProjectAsset = {};
            serializedIt->ManagedScriptTypeName.clear();
        }
        else if (const auto* managedScript = dynamic_cast<const Pragma::Renderer::ManagedScriptComponent*>(runtimeObject.GetBehaviour());
            managedScript != nullptr)
        {
            serializedIt->ScriptName.clear();
            serializedIt->ManagedScriptProjectAsset = managedScript->GetProjectAssetId();
            serializedIt->ManagedScriptTypeName = managedScript->GetTypeName();
        }
        else
        {
            serializedIt->ScriptName.clear();
            serializedIt->ManagedScriptProjectAsset = {};
            serializedIt->ManagedScriptTypeName.clear();
        }

        if (const auto* light = runtimeObject.GetLight(); light != nullptr)
        {
            serializedIt->Light = *light;
        }
        else
        {
            serializedIt->Light.reset();
        }

        if (const auto* rigidBody = runtimeObject.GetRigidBody(); rigidBody != nullptr)
        {
            serializedIt->RigidBody = *rigidBody;
        }
        else
        {
            serializedIt->RigidBody.reset();
        }

        if (const auto* boxCollider = runtimeObject.GetBoxCollider(); boxCollider != nullptr)
        {
            serializedIt->BoxCollider = *boxCollider;
        }
        else
        {
            serializedIt->BoxCollider.reset();
        }
    }

    return snapshot;
}

bool SceneStateBridge::AreEqual(const SerializedScene& lhs, const SerializedScene& rhs) noexcept
{
    if (lhs.Version != rhs.Version || lhs.Objects.size() != rhs.Objects.size())
    {
        return false;
    }

    for (std::size_t index = 0; index < lhs.Objects.size(); ++index)
    {
        if (!AreSceneObjectsEqual(lhs.Objects[index], rhs.Objects[index]))
        {
            return false;
        }
    }

    return true;
}

std::string SceneStateBridge::DescribeDifference(const SerializedScene& expected, const SerializedScene& actual)
{
    if (expected.Version != actual.Version)
    {
        return "Scene version mismatch after rebuild.";
    }

    if (expected.Objects.size() != actual.Objects.size())
    {
        return "Scene object count mismatch after rebuild.";
    }

    for (std::size_t index = 0; index < expected.Objects.size(); ++index)
    {
        const SerializedSceneObject& expectedObject = expected.Objects[index];
        const SerializedSceneObject& actualObject = actual.Objects[index];
        if (AreSceneObjectsEqual(expectedObject, actualObject))
        {
            continue;
        }

        const std::string objectPrefix =
            "Scene object mismatch after rebuild for EntityId=" + std::to_string(expectedObject.Id) + ": ";

        if (expectedObject.Id != actualObject.Id)
        {
            return objectPrefix + "id differs.";
        }
        if (expectedObject.ParentId != actualObject.ParentId)
        {
            return objectPrefix + "parent differs.";
        }
        if (expectedObject.Name != actualObject.Name)
        {
            return objectPrefix + "name differs.";
        }
        if (!AreAssetIdsEqual(expectedObject.PrefabAssetId, actualObject.PrefabAssetId))
        {
            return objectPrefix + "prefab asset differs.";
        }
        if (!AreTransformsEqual(expectedObject.Transform, actualObject.Transform))
        {
            return objectPrefix + "local transform differs.";
        }
        if (expectedObject.MeshRenderer.has_value() != actualObject.MeshRenderer.has_value())
        {
            return objectPrefix + "mesh renderer presence differs.";
        }
        if (expectedObject.MeshRenderer.has_value() && !AreMeshRenderersEqual(*expectedObject.MeshRenderer, *actualObject.MeshRenderer))
        {
            return objectPrefix + "mesh renderer data differs.";
        }
        if (expectedObject.Camera.has_value() != actualObject.Camera.has_value())
        {
            return objectPrefix + "camera presence differs.";
        }
        if (expectedObject.Camera.has_value() && (
            expectedObject.Camera->PitchRadians != actualObject.Camera->PitchRadians ||
            expectedObject.Camera->FieldOfViewRadians != actualObject.Camera->FieldOfViewRadians ||
            expectedObject.Camera->NearPlane != actualObject.Camera->NearPlane ||
            expectedObject.Camera->FarPlane != actualObject.Camera->FarPlane))
        {
            return objectPrefix + "camera data differs.";
        }
        if (expectedObject.CameraController.has_value() != actualObject.CameraController.has_value())
        {
            return objectPrefix + "camera controller presence differs.";
        }
        if (expectedObject.CameraController.has_value() &&
            !AreCameraControllersEqual(*expectedObject.CameraController, *actualObject.CameraController))
        {
            return objectPrefix + "camera controller data differs.";
        }
        if (expectedObject.Light.has_value() != actualObject.Light.has_value())
        {
            return objectPrefix + "light presence differs.";
        }
        if (expectedObject.Light.has_value() && !AreLightsEqual(*expectedObject.Light, *actualObject.Light))
        {
            return objectPrefix + "light data differs.";
        }
        if (expectedObject.RigidBody.has_value() != actualObject.RigidBody.has_value())
        {
            return objectPrefix + "rigid body presence differs.";
        }
        if (expectedObject.RigidBody.has_value() && !AreRigidBodiesEqual(*expectedObject.RigidBody, *actualObject.RigidBody))
        {
            return objectPrefix + "rigid body data differs.";
        }
        if (expectedObject.BoxCollider.has_value() != actualObject.BoxCollider.has_value())
        {
            return objectPrefix + "box collider presence differs.";
        }
        if (expectedObject.BoxCollider.has_value() && !AreBoxCollidersEqual(*expectedObject.BoxCollider, *actualObject.BoxCollider))
        {
            return objectPrefix + "box collider data differs.";
        }
        if (expectedObject.ScriptName != actualObject.ScriptName)
        {
            return objectPrefix + "script binding differs.";
        }
        if (!AreAssetIdsEqual(expectedObject.ManagedScriptProjectAsset, actualObject.ManagedScriptProjectAsset))
        {
            return objectPrefix + "managed script project differs.";
        }
        if (expectedObject.ManagedScriptTypeName != actualObject.ManagedScriptTypeName)
        {
            return objectPrefix + "managed script type differs.";
        }
        if (expectedObject.IsActiveCamera != actualObject.IsActiveCamera)
        {
            return objectPrefix + "active camera flag differs.";
        }

        return objectPrefix + "unknown difference.";
    }

    return {};
}
}
