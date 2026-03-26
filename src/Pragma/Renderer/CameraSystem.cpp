#include "Pragma/Renderer/CameraSystem.h"

#include "Pragma/Core/Assert.h"
#include "Pragma/Core/Profiler.h"
#include "Pragma/Renderer/Camera.h"
#include "Pragma/Renderer/CameraControllerComponent.h"
#include "Pragma/Renderer/Scene.h"

namespace Pragma::Renderer
{
void CameraSystem::Update(Scene& scene, const Pragma::Core::EngineTime& time, const Pragma::Core::EngineInput& input)
{
    PRAGMA_PROFILE_SCOPE("CameraSystem::Update");
    SceneObject* activeCameraObject = scene.GetActiveCameraObject();
    if (activeCameraObject == nullptr || !activeCameraObject->HasCamera())
    {
        return;
    }

    CameraControllerComponent* cameraController = activeCameraObject->GetCameraController();
    if (cameraController == nullptr || !cameraController->Enabled)
    {
        return;
    }

    CameraComponent* cameraComponent = activeCameraObject->GetCamera();
    PRAGMA_ASSERT(cameraComponent != nullptr, "Active camera object is missing CameraComponent.");
    const EntityId activeCameraId = activeCameraObject->Id;

    const float moveSpeed = input.IsFastMovePressed() ? cameraController->FastMoveSpeed : cameraController->MoveSpeed;

    float moveForward = 0.0f;
    float moveRight = 0.0f;
    float moveUp = 0.0f;
    float yaw = 0.0f;
    float pitch = 0.0f;

    if (input.IsMoveForwardPressed()) { moveForward += 1.0f; }
    if (input.IsMoveBackwardPressed()) { moveForward -= 1.0f; }
    if (input.IsMoveRightPressed()) { moveRight += 1.0f; }
    if (input.IsMoveLeftPressed()) { moveRight -= 1.0f; }
    if (input.IsMoveUpPressed()) { moveUp += 1.0f; }
    if (input.IsMoveDownPressed()) { moveUp -= 1.0f; }

    if (input.IsLookRightPressed()) { yaw -= 1.0f; }
    if (input.IsLookLeftPressed()) { yaw += 1.0f; }
    if (input.IsLookUpPressed()) { pitch += 1.0f; }
    if (input.IsLookDownPressed()) { pitch -= 1.0f; }

    Camera runtimeCamera;
    const Transform worldTransform = scene.GetWorldTransform(activeCameraId);
    runtimeCamera.SetPose(
        worldTransform.Position,
        worldTransform.RotationRadians.Y,
        cameraComponent->PitchRadians);

    runtimeCamera.MoveLocal(
        moveForward * moveSpeed * time.DeltaSeconds,
        moveRight * moveSpeed * time.DeltaSeconds,
        moveUp * moveSpeed * time.DeltaSeconds);
    runtimeCamera.Rotate(
        yaw * cameraController->KeyboardLookSpeed * time.DeltaSeconds,
        pitch * cameraController->KeyboardLookSpeed * time.DeltaSeconds);

    if (input.IsRightMouseButtonDown())
    {
        runtimeCamera.Rotate(
            static_cast<float>(-input.GetMouseDeltaX()) * cameraController->MouseLookSensitivity,
            static_cast<float>(-input.GetMouseDeltaY()) * cameraController->MouseLookSensitivity);
    }

    Transform updatedWorldTransform = worldTransform;
    updatedWorldTransform.Position = runtimeCamera.GetPosition();
    updatedWorldTransform.RotationRadians.Y = runtimeCamera.GetYawRadians();
    (void)scene.SetWorldTransform(activeCameraId, updatedWorldTransform);
    cameraComponent->PitchRadians = runtimeCamera.GetPitchRadians();
}
}
