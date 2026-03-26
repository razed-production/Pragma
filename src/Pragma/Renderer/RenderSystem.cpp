#include "Pragma/Renderer/RenderSystem.h"

#include "Pragma/Renderer/Camera.h"
#include "Pragma/Renderer/Primitives.h"
#include "Pragma/Core/Assert.h"
#include "Pragma/Core/Log.h"
#include "Pragma/Core/Profiler.h"
#include "Pragma/Math/Matrix4.h"
#include "Pragma/RHI/CommandList.h"
#include "Pragma/RHI/Device.h"
#include "Pragma/RHI/Resources.h"
#include "Pragma/RHI/Swapchain.h"

#include <algorithm>

namespace Pragma::Renderer
{
namespace
{
struct alignas(16) FrameConstants
{
    Pragma::Math::Matrix4 WorldViewProjection;
    Pragma::Math::Matrix4 World;
    float LightDirection[3]{ -0.4f, -0.8f, 0.3f };
    float LightIntensity = 1.0f;
    float LightColor[3]{ 1.0f, 1.0f, 1.0f };
    float Padding0 = 0.0f;
};

constexpr float kPi = 3.1415926535f;
}

RenderSystem::RenderSystem(Pragma::RHI::IDevice& device, const Pragma::RHI::NativeWindow window, const Pragma::RHI::Extent2D extent)
    : m_device(device)
{
    m_swapchainDesc.Extent = extent;
    m_swapchainDesc.Window = window;
    m_swapchainDesc.BufferCount = 2;
    m_swapchainDesc.VSyncEnabled = true;

    Pragma::RHI::BufferDesc constantBufferDesc;
    constantBufferDesc.SizeInBytes = sizeof(FrameConstants);
    constantBufferDesc.Stride = sizeof(FrameConstants);
    constantBufferDesc.BindMask = Pragma::RHI::Bind_ConstantBuffer;
    constantBufferDesc.Usage = Pragma::RHI::ResourceUsage::Dynamic;
    constantBufferDesc.CpuWritable = true;

    m_frameConstantBuffer = m_device.CreateBuffer(constantBufferDesc, nullptr);
    m_debugCubeMesh = CreateCubeMesh(m_device);
    m_physicsDebugMaterial = CreateDefaultMaterial(m_device);
    m_physicsDebugMaterial->Parameters.UseAlbedoTexture = 0.0f;
    Resize(extent);
}

RenderSystem::~RenderSystem() = default;

void RenderSystem::Initialize()
{
    m_swapchain = m_device.CreateSwapchain(m_swapchainDesc);
    m_commandList = m_device.CreateCommandList();

    Pragma::Core::Log(
        Pragma::Core::LogCategory::Renderer,
        Pragma::Core::LogLevel::Info,
        "Renderer initialized on " + std::string(Pragma::RHI::ToString(m_device.GetBackendType())));
    Pragma::Core::Log(
        Pragma::Core::LogCategory::Renderer,
        Pragma::Core::LogLevel::Info,
        "Default swapchain extent: " + std::to_string(m_swapchainDesc.Extent.Width) + "x" + std::to_string(m_swapchainDesc.Extent.Height));
}

void RenderSystem::Resize(const Pragma::RHI::Extent2D extent)
{
    if (extent.Width == 0 || extent.Height == 0)
    {
        return;
    }

    const bool changed =
        m_swapchainDesc.Extent.Width != extent.Width ||
        m_swapchainDesc.Extent.Height != extent.Height;

    m_swapchainDesc.Extent = extent;

    if (changed && m_swapchain != nullptr)
    {
        m_swapchain->Resize(extent);
    }
}

void RenderSystem::RenderFrame(
    const Scene& scene,
    const bool showPhysicsOverlay,
    const std::function<void()>& overlayCallback)
{
    PRAGMA_PROFILE_SCOPE("RenderSystem::RenderFrame");
    PRAGMA_ASSERT(m_commandList != nullptr, "RenderSystem command list is not initialized.");
    PRAGMA_ASSERT(m_swapchain != nullptr, "RenderSystem swapchain is not initialized.");
    PRAGMA_ASSERT(scene.IsInitialized(), "RenderSystem received a scene that has not been initialized.");
    PRAGMA_ASSERT(!scene.GetObjects().empty(), "RenderSystem received an empty scene.");
    PRAGMA_ASSERT(scene.GetActiveCameraEntityId() != InvalidEntityId, "RenderSystem received a scene without an active camera.");

    {
        PRAGMA_PROFILE_SCOPE("Render Begin");
        m_commandList->Begin();
        m_commandList->SetRenderTargets(nullptr, nullptr);
        m_commandList->ClearColor({ 0.08f, 0.12f, 0.18f, 1.0f });
        m_commandList->ClearDepth(1.0f);
        m_commandList->SetConstantBuffer(0, *m_frameConstantBuffer);
    }

    DrawSceneToCurrentTargets(scene, m_swapchainDesc.Extent, showPhysicsOverlay);

    {
        PRAGMA_PROFILE_SCOPE("Render Submit");
        m_commandList->End();
        m_device.Submit(*m_commandList);
    }

    if (overlayCallback)
    {
        PRAGMA_PROFILE_SCOPE("Overlay Render");
        overlayCallback();
    }

    {
        PRAGMA_PROFILE_SCOPE("Present");
        m_swapchain->Present();
    }
}

void RenderSystem::DrawSceneToCurrentTargets(const Scene& scene, const Pragma::RHI::Extent2D extent, const bool showPhysicsOverlay)
{
    PRAGMA_ASSERT(scene.IsInitialized(), "RenderSystem received a scene that has not been initialized.");
    PRAGMA_ASSERT(!scene.GetObjects().empty(), "RenderSystem received an empty scene.");
    PRAGMA_ASSERT(scene.GetActiveCameraEntityId() != InvalidEntityId, "RenderSystem received a scene without an active camera.");

    const SceneObject* activeCameraObject = scene.GetActiveCameraObject();
    PRAGMA_ASSERT(activeCameraObject != nullptr, "RenderSystem failed to resolve the active camera object.");
    PRAGMA_ASSERT(activeCameraObject->HasCamera(), "Active camera object is missing CameraComponent.");
    const CameraComponent* activeCameraComponent = activeCameraObject->GetCamera();
    PRAGMA_ASSERT(activeCameraComponent != nullptr, "Active camera object returned a null CameraComponent.");

    const float aspectRatio = static_cast<float>(extent.Width) / static_cast<float>(extent.Height);
    Camera activeCamera;
    activeCamera.SetPerspective(
        activeCameraComponent->FieldOfViewRadians,
        aspectRatio,
        activeCameraComponent->NearPlane,
        activeCameraComponent->FarPlane);
    const Transform activeCameraWorldTransform = scene.GetWorldTransform(activeCameraObject->Id);
    activeCamera.SetPose(
        activeCameraWorldTransform.Position,
        activeCameraWorldTransform.RotationRadians.Y,
        activeCameraComponent->PitchRadians);

    const Pragma::Math::Matrix4 viewProjection = activeCamera.GetViewProjection();
    FrameConstants lightingConstants{};

    for (const SceneObject& object : scene.GetObjects())
    {
        const LightComponent* light = object.GetLight();
        if (light == nullptr)
        {
            continue;
        }

        lightingConstants.LightDirection[0] = light->Direction[0];
        lightingConstants.LightDirection[1] = light->Direction[1];
        lightingConstants.LightDirection[2] = light->Direction[2];
        lightingConstants.LightIntensity = light->Intensity;
        lightingConstants.LightColor[0] = light->Color[0];
        lightingConstants.LightColor[1] = light->Color[1];
        lightingConstants.LightColor[2] = light->Color[2];
        break;
    }

    static bool hasLoggedFirstFrame = false;
    if (!hasLoggedFirstFrame)
    {
        hasLoggedFirstFrame = true;
        Pragma::Core::Log(
            Pragma::Core::LogCategory::Renderer,
            Pragma::Core::LogLevel::Info,
            "First render frame: " +
            std::to_string(scene.GetObjects().size()) + " objects.");
    }

    PRAGMA_PROFILE_SCOPE("Draw Scene");
    for (const SceneObject& object : scene.GetObjects())
    {
        const MeshRendererComponent* meshRenderer = object.GetMeshRenderer();
        if (meshRenderer == nullptr ||
            meshRenderer->Mesh == nullptr ||
            meshRenderer->Material == nullptr ||
            meshRenderer->Material->Pipeline == nullptr)
        {
            continue;
        }

        FrameConstants frameConstants = lightingConstants;
        const Pragma::Math::Matrix4 world = ToMatrix(scene.GetWorldTransform(object.Id));
        frameConstants.WorldViewProjection = Pragma::Math::Multiply(world, viewProjection);
        frameConstants.World = world;
        m_device.UpdateBuffer(*m_frameConstantBuffer, &frameConstants, sizeof(frameConstants));

        m_commandList->SetGraphicsPipeline(*meshRenderer->Material->Pipeline);
        m_commandList->SetVertexBuffer(*meshRenderer->Mesh->VertexBuffer, 0);
        m_commandList->SetIndexBuffer(*meshRenderer->Mesh->IndexBuffer, meshRenderer->Mesh->IndexFormat, 0);
        m_commandList->SetConstantBuffer(1, *meshRenderer->Material->ParametersBuffer);
        m_commandList->SetTexture(0, meshRenderer->Material->AlbedoTexture.get());
        m_commandList->DrawIndexed(meshRenderer->Mesh->IndexCount, 0, 0);
    }

    if (!showPhysicsOverlay || m_debugCubeMesh == nullptr || m_physicsDebugMaterial == nullptr)
    {
        return;
    }

    PRAGMA_PROFILE_SCOPE("Draw Physics Overlay");
    for (const SceneObject& object : scene.GetObjects())
    {
        const auto* boxCollider = object.GetBoxCollider();
        const auto* rigidBody = object.GetRigidBody();
        if (boxCollider == nullptr && rigidBody == nullptr)
        {
            continue;
        }

        const bool incompletePhysicsSetup = boxCollider == nullptr || rigidBody == nullptr;
        const bool invalidCollider =
            boxCollider != nullptr &&
            (boxCollider->HalfExtent.X <= 0.0f || boxCollider->HalfExtent.Y <= 0.0f || boxCollider->HalfExtent.Z <= 0.0f);

        Transform worldTransform = scene.GetWorldTransform(object.Id);
        if (boxCollider != nullptr)
        {
            worldTransform.Scale.X *= boxCollider->HalfExtent.X;
            worldTransform.Scale.Y *= boxCollider->HalfExtent.Y;
            worldTransform.Scale.Z *= boxCollider->HalfExtent.Z;
        }

        if (invalidCollider || incompletePhysicsSetup)
        {
            m_physicsDebugMaterial->Parameters.BaseColor[0] = 1.0f;
            m_physicsDebugMaterial->Parameters.BaseColor[1] = 0.2f;
            m_physicsDebugMaterial->Parameters.BaseColor[2] = 0.2f;
            m_physicsDebugMaterial->Parameters.BaseColor[3] = 1.0f;
        }
        else if (rigidBody->MotionType == Pragma::Renderer::RigidBodyMotionType::Static)
        {
            m_physicsDebugMaterial->Parameters.BaseColor[0] = 0.2f;
            m_physicsDebugMaterial->Parameters.BaseColor[1] = 0.9f;
            m_physicsDebugMaterial->Parameters.BaseColor[2] = 1.0f;
            m_physicsDebugMaterial->Parameters.BaseColor[3] = 1.0f;
        }
        else
        {
            m_physicsDebugMaterial->Parameters.BaseColor[0] = 1.0f;
            m_physicsDebugMaterial->Parameters.BaseColor[1] = 0.8f;
            m_physicsDebugMaterial->Parameters.BaseColor[2] = 0.2f;
            m_physicsDebugMaterial->Parameters.BaseColor[3] = 1.0f;
        }
        m_physicsDebugMaterial->Parameters.Roughness = 0.0f;
        m_physicsDebugMaterial->Parameters.UseAlbedoTexture = 0.0f;
        m_device.UpdateBuffer(
            *m_physicsDebugMaterial->ParametersBuffer,
            &m_physicsDebugMaterial->Parameters,
            sizeof(m_physicsDebugMaterial->Parameters));

        FrameConstants frameConstants = lightingConstants;
        const Pragma::Math::Matrix4 world = ToMatrix(worldTransform);
        frameConstants.WorldViewProjection = Pragma::Math::Multiply(world, viewProjection);
        frameConstants.World = world;
        m_device.UpdateBuffer(*m_frameConstantBuffer, &frameConstants, sizeof(frameConstants));

        m_commandList->SetGraphicsPipeline(*m_physicsDebugMaterial->Pipeline);
        m_commandList->SetVertexBuffer(*m_debugCubeMesh->VertexBuffer, 0);
        m_commandList->SetIndexBuffer(*m_debugCubeMesh->IndexBuffer, m_debugCubeMesh->IndexFormat, 0);
        m_commandList->SetConstantBuffer(1, *m_physicsDebugMaterial->ParametersBuffer);
        m_commandList->SetTexture(0, nullptr);
        m_commandList->DrawIndexed(m_debugCubeMesh->IndexCount, 0, 0);
    }
}

Pragma::RHI::BackendType RenderSystem::GetBackendType() const noexcept
{
    return m_device.GetBackendType();
}
}
