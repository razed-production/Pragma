#include "Pragma/Renderer/RenderSystem.h"

#include "Pragma/Renderer/Camera.h"
#include "Pragma/Renderer/Primitives.h"
#include "Pragma/Renderer/ShaderSource.h"
#include "Pragma/Renderer/Vertex.h"
#include "Pragma/Core/Assert.h"
#include "Pragma/Core/Log.h"
#include "Pragma/Core/Profiler.h"
#include "Pragma/Math/Matrix4.h"
#include "Pragma/Math/Vector3.h"
#include "Pragma/RHI/CommandList.h"
#include "Pragma/RHI/Device.h"
#include "Pragma/RHI/Resources.h"
#include "Pragma/RHI/Swapchain.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace Pragma::Renderer
{
namespace
{
struct ShadowQualitySettings
{
    std::uint32_t MapResolution = 2048;
    float Distance = 24.0f;
    float HalfExtent = 18.0f;
    float NearPlane = 1.0f;
    float FarPlane = 64.0f;
    float Bias = 0.0015f;
};

struct Plane
{
    Pragma::Math::Vector3 Normal;
    float Distance = 0.0f;
};

struct Frustum
{
    Plane Left;
    Plane Right;
    Plane Bottom;
    Plane Top;
    Plane Near;
    Plane Far;
};

struct BoundingSphere
{
    Pragma::Math::Vector3 Center;
    float Radius = 0.0f;
};

enum class LodLevel
{
    High,
    Medium,
    Low
};

struct RenderProxy
{
    const Mesh* Mesh = nullptr;
    const Material* Material = nullptr;
    BoundingSphere WorldBounds{};
    Pragma::Math::Matrix4 World;
    Pragma::Math::Matrix4 WorldNoScale;
    Pragma::Math::Matrix4 WorldViewProjection;
    Pragma::Math::Matrix4 WorldLightClip;
    float CameraDistance = 0.0f;
    LodLevel Level = LodLevel::High;
    bool MainVisible = false;
    bool CastsShadow = false;
};

struct WorldTransformCache
{
    std::unordered_map<EntityId, std::size_t> ObjectIndices;
    std::vector<Transform> WorldTransforms;
    std::vector<bool> Computed;
};

struct ShadowRenderItem
{
    const Mesh* Mesh = nullptr;
    Pragma::Math::Matrix4 WorldLightClip;
};

struct MainRenderItem
{
    const Mesh* Mesh = nullptr;
    const Material* Material = nullptr;
    Pragma::Math::Matrix4 World;
    Pragma::Math::Matrix4 WorldNoScale;
    Pragma::Math::Matrix4 WorldViewProjection;
    Pragma::Math::Matrix4 WorldLightClip;
};

constexpr std::uint32_t kMaxBatchedInstances = 64;

struct alignas(16) FrameConstants
{
    float CameraPosition[3]{ 0.0f, 0.0f, 0.0f };
    float Exposure = 1.0f;
    float LightDirection[3]{ -0.4f, -0.8f, 0.3f };
    float LightIntensity = 1.0f;
    float LightColor[3]{ 1.0f, 1.0f, 1.0f };
    float AmbientStrength = 0.26f;
    float SkyZenithColor[3]{ 0.07f, 0.16f, 0.38f };
    float EnvironmentDiffuseStrength = 0.92f;
    float SkyHorizonColor[3]{ 0.84f, 0.89f, 1.0f };
    float EnvironmentSpecularStrength = 1.08f;
    float SkyGroundColor[3]{ 0.07f, 0.075f, 0.09f };
    float SkyAtmosphereDensity = 0.22f;
    float FogColor[3]{ 0.76f, 0.84f, 0.96f };
    float FogStartDistance = 18.0f;
    float FogDensity = 0.03f;
    float FogHeightFalloff = 0.09f;
    float FogMaxOpacity = 0.72f;
    float Padding0 = 0.0f;
    float ShadowMapTexelSize[2]{ 0.0f, 0.0f };
    float ShadowBias = 0.0015f;
    float ShadowStrength = 0.0f;
    float ShadowFilterQuality = 0.0f;
    float ShadingQuality = 1.0f;
    float Padding1[2]{ 0.0f, 0.0f };
};

struct alignas(16) ShadowConstants
{
    Pragma::Math::Matrix4 WorldLightClip[kMaxBatchedInstances];
};

struct alignas(16) MainInstanceConstants
{
    Pragma::Math::Matrix4 WorldViewProjection[kMaxBatchedInstances];
    Pragma::Math::Matrix4 World[kMaxBatchedInstances];
    Pragma::Math::Matrix4 WorldNoScale[kMaxBatchedInstances];
    Pragma::Math::Matrix4 WorldLightClip[kMaxBatchedInstances];
};

struct alignas(16) TonemapConstants
{
    float Exposure = 1.0f;
    float BloomIntensity = 0.0f;
    float FxaaEnabled = 0.0f;
    float FxaaSubpixel = 0.75f;
    float FxaaEdgeThreshold = 0.166f;
    float FxaaEdgeThresholdMin = 0.0625f;
    float SourceTexelSize[2]{ 0.0f, 0.0f };
};

struct alignas(16) SkyConstants
{
    float CameraForward[3]{ 0.0f, 0.0f, 1.0f };
    float TanHalfFovY = 0.577f;
    float CameraRight[3]{ 1.0f, 0.0f, 0.0f };
    float AspectRatio = 1.777f;
    float CameraUp[3]{ 0.0f, 1.0f, 0.0f };
    float SunIntensity = 1.0f;
    float SunDirection[3]{ 0.0f, 1.0f, 0.0f };
    float Padding0 = 0.0f;
    float ZenithColor[3]{ 0.08f, 0.18f, 0.42f };
    float Padding1 = 0.0f;
    float HorizonColor[3]{ 0.72f, 0.82f, 1.0f };
    float Padding2 = 0.0f;
    float GroundColor[3]{ 0.05f, 0.06f, 0.07f };
    float Padding3 = 0.0f;
    float SunColor[3]{ 1.0f, 0.95f, 0.85f };
    float SunAngularSize = 0.0045f;
    float HorizonGlowStrength = 0.14f;
    float AtmosphereDensity = 0.22f;
    float GroundBounceStrength = 0.08f;
    float Padding4 = 0.0f;
};

struct alignas(16) BloomConstants
{
    float Threshold = 1.1f;
    float Intensity = 0.08f;
    float Quality = 1.0f;
    float Padding0 = 0.0f;
    float SourceTexelSize[2]{ 0.0f, 0.0f };
    float Padding1[2]{ 0.0f, 0.0f };
};

ShadowQualitySettings ResolveShadowQualitySettings(const Pragma::Core::ShadowQualityTier tier) noexcept
{
    switch (tier)
    {
    case Pragma::Core::ShadowQualityTier::Low:
        return { 1024u, 18.0f, 14.0f, 1.0f, 48.0f, 0.0022f };
    case Pragma::Core::ShadowQualityTier::Medium:
        return { 1536u, 22.0f, 16.0f, 1.0f, 56.0f, 0.0018f };
    case Pragma::Core::ShadowQualityTier::Ultra:
        return { 3072u, 30.0f, 22.0f, 1.0f, 72.0f, 0.0012f };
    case Pragma::Core::ShadowQualityTier::High:
    default:
        return { 2048u, 24.0f, 18.0f, 1.0f, 64.0f, 0.0015f };
    }
}

void ApplyGraphicsQualityPreset(Pragma::Core::GraphicsConfig& graphicsConfig) noexcept
{
    switch (graphicsConfig.QualityPreset)
    {
    case Pragma::Core::GraphicsQualityPreset::Performance:
        graphicsConfig.RenderScale = 0.70f;
        graphicsConfig.ShadingQuality = Pragma::Core::ShadingQualityTier::Performance;
        graphicsConfig.BloomEnabled = false;
        graphicsConfig.BloomResolutionScale = 0.25f;
        graphicsConfig.BloomQuality = 0.0f;
        graphicsConfig.BloomThreshold = 1.2f;
        graphicsConfig.BloomIntensity = 0.0f;
        graphicsConfig.FxaaEnabled = false;
        graphicsConfig.FxaaSubpixel = 0.60f;
        graphicsConfig.FxaaEdgeThreshold = 0.20f;
        graphicsConfig.FxaaEdgeThresholdMin = 0.0833f;
        graphicsConfig.ShadowFilterQuality = 0.0f;
        break;
    case Pragma::Core::GraphicsQualityPreset::Quality:
        graphicsConfig.RenderScale = 1.0f;
        graphicsConfig.ShadingQuality = Pragma::Core::ShadingQualityTier::Quality;
        graphicsConfig.BloomEnabled = true;
        graphicsConfig.BloomResolutionScale = 0.5f;
        graphicsConfig.BloomQuality = 2.0f;
        graphicsConfig.BloomThreshold = 1.0f;
        graphicsConfig.BloomIntensity = 0.10f;
        graphicsConfig.FxaaEnabled = true;
        graphicsConfig.FxaaSubpixel = 0.75f;
        graphicsConfig.FxaaEdgeThreshold = 0.140f;
        graphicsConfig.FxaaEdgeThresholdMin = 0.050f;
        graphicsConfig.ShadowFilterQuality = 1.0f;
        break;
    case Pragma::Core::GraphicsQualityPreset::Custom:
        break;
    case Pragma::Core::GraphicsQualityPreset::Balanced:
    default:
        graphicsConfig.RenderScale = 0.85f;
        graphicsConfig.ShadingQuality = Pragma::Core::ShadingQualityTier::Balanced;
        graphicsConfig.BloomEnabled = true;
        graphicsConfig.BloomResolutionScale = 0.125f;
        graphicsConfig.BloomQuality = 0.0f;
        graphicsConfig.BloomThreshold = 1.15f;
        graphicsConfig.BloomIntensity = 0.05f;
        graphicsConfig.FxaaEnabled = false;
        graphicsConfig.FxaaSubpixel = 0.75f;
        graphicsConfig.FxaaEdgeThreshold = 0.166f;
        graphicsConfig.FxaaEdgeThresholdMin = 0.0625f;
        graphicsConfig.ShadowFilterQuality = 0.0f;
        break;
    }
}

void SanitizeGraphicsConfig(Pragma::Core::GraphicsConfig& graphicsConfig) noexcept
{
    graphicsConfig.RenderScale = std::clamp(graphicsConfig.RenderScale, 0.5f, 1.0f);
    graphicsConfig.Exposure = std::clamp(graphicsConfig.Exposure, 0.25f, 4.0f);
    graphicsConfig.AmbientStrength = std::clamp(graphicsConfig.AmbientStrength, 0.0f, 2.0f);
    graphicsConfig.EnvironmentDiffuseStrength = std::clamp(graphicsConfig.EnvironmentDiffuseStrength, 0.0f, 2.0f);
    graphicsConfig.EnvironmentSpecularStrength = std::clamp(graphicsConfig.EnvironmentSpecularStrength, 0.0f, 2.5f);
    graphicsConfig.BloomResolutionScale = std::clamp(graphicsConfig.BloomResolutionScale, 0.125f, 0.5f);
    graphicsConfig.BloomQuality = std::clamp(graphicsConfig.BloomQuality, 0.0f, 2.0f);
    graphicsConfig.BloomThreshold = std::clamp(graphicsConfig.BloomThreshold, 0.5f, 4.0f);
    graphicsConfig.BloomIntensity = std::clamp(graphicsConfig.BloomIntensity, 0.0f, 1.0f);
    graphicsConfig.FxaaSubpixel = std::clamp(graphicsConfig.FxaaSubpixel, 0.0f, 1.0f);
    graphicsConfig.FxaaEdgeThreshold = std::clamp(graphicsConfig.FxaaEdgeThreshold, 0.0312f, 0.333f);
    graphicsConfig.FxaaEdgeThresholdMin = std::clamp(graphicsConfig.FxaaEdgeThresholdMin, 0.0f, 0.125f);
    graphicsConfig.ShadowFilterQuality = std::clamp(graphicsConfig.ShadowFilterQuality, 0.0f, 1.0f);
    graphicsConfig.FogStartDistance = std::clamp(graphicsConfig.FogStartDistance, 0.0f, 256.0f);
    graphicsConfig.FogDensity = std::clamp(graphicsConfig.FogDensity, 0.0f, 0.25f);
    graphicsConfig.FogHeightFalloff = std::clamp(graphicsConfig.FogHeightFalloff, 0.0f, 0.5f);
    graphicsConfig.FogMaxOpacity = std::clamp(graphicsConfig.FogMaxOpacity, 0.0f, 1.0f);
    graphicsConfig.LodNearNormalizedDistance = std::max(0.1f, graphicsConfig.LodNearNormalizedDistance);
    graphicsConfig.LodFarNormalizedDistance = std::max(
        graphicsConfig.LodNearNormalizedDistance + 0.1f,
        graphicsConfig.LodFarNormalizedDistance);
    graphicsConfig.ShadowLowLodDistanceScale = std::clamp(graphicsConfig.ShadowLowLodDistanceScale, 0.1f, 2.0f);
}

float ToShaderQualityValue(const Pragma::Core::ShadingQualityTier tier) noexcept
{
    switch (tier)
    {
    case Pragma::Core::ShadingQualityTier::Performance:
        return 0.0f;
    case Pragma::Core::ShadingQualityTier::Quality:
        return 2.0f;
    case Pragma::Core::ShadingQualityTier::Balanced:
    default:
        return 1.0f;
    }
}

Pragma::RHI::Extent2D ResolveInternalRenderExtent(
    const Pragma::RHI::Extent2D outputExtent,
    const float renderScale) noexcept
{
    const float clampedScale = std::clamp(renderScale, 0.5f, 1.0f);
    const std::uint32_t width = std::max(1u, static_cast<std::uint32_t>(std::lround(static_cast<double>(outputExtent.Width) * clampedScale)));
    const std::uint32_t height = std::max(1u, static_cast<std::uint32_t>(std::lround(static_cast<double>(outputExtent.Height) * clampedScale)));
    return { width, height };
}

const char* ToString(const Pragma::Core::ShadowQualityTier tier) noexcept
{
    switch (tier)
    {
    case Pragma::Core::ShadowQualityTier::Low:
        return "Low";
    case Pragma::Core::ShadowQualityTier::Medium:
        return "Medium";
    case Pragma::Core::ShadowQualityTier::Ultra:
        return "Ultra";
    case Pragma::Core::ShadowQualityTier::High:
    default:
        return "High";
    }
}

std::unique_ptr<Pragma::RHI::IPipelineState> CreateFullscreenPipeline(
    Pragma::RHI::IDevice& device,
    const char* debugName,
    const char* pixelShaderFile,
    const Pragma::RHI::PixelFormat colorFormat)
{
    static const std::string s_vertexShaderSource = Pragma::Renderer::LoadRendererShaderSource("tonemap_vs.hlsl");
    const std::string pixelShaderSource = Pragma::Renderer::LoadRendererShaderSource(pixelShaderFile);

    Pragma::RHI::GraphicsPipelineDesc pipelineDesc;
    pipelineDesc.DebugName = debugName;
    pipelineDesc.ColorFormat = colorFormat;
    pipelineDesc.DepthFormat = Pragma::RHI::PixelFormat::D24_UNorm_S8_UInt;
    pipelineDesc.Rasterizer.Cull = Pragma::RHI::CullMode::None;
    pipelineDesc.DepthStencil.DepthEnabled = false;
    pipelineDesc.DepthStencil.DepthWriteEnabled = false;
    pipelineDesc.VertexStride = 0;
    pipelineDesc.VertexShader.Stage = Pragma::RHI::ShaderStage::Vertex;
    pipelineDesc.VertexShader.SourceCode = s_vertexShaderSource.c_str();
    pipelineDesc.VertexShader.DebugName = "FullscreenTriangleVS";
    pipelineDesc.PixelShader.Stage = Pragma::RHI::ShaderStage::Pixel;
    pipelineDesc.PixelShader.SourceCode = pixelShaderSource.c_str();
    pipelineDesc.PixelShader.DebugName = debugName;

    return device.CreateGraphicsPipeline(pipelineDesc);
}

std::unique_ptr<Pragma::RHI::IPipelineState> CreateTonemapPipeline(Pragma::RHI::IDevice& device)
{
    return CreateFullscreenPipeline(device, "TonemapPS", "tonemap_ps.hlsl", Pragma::RHI::PixelFormat::R8G8B8A8_UNorm);
}

std::unique_ptr<Pragma::RHI::IPipelineState> CreateSkyPipeline(Pragma::RHI::IDevice& device)
{
    return CreateFullscreenPipeline(device, "SkyPS", "sky_ps.hlsl", Pragma::RHI::PixelFormat::R16G16B16A16_Float);
}

std::unique_ptr<Pragma::RHI::IPipelineState> CreateBloomPipeline(Pragma::RHI::IDevice& device)
{
    return CreateFullscreenPipeline(device, "BloomPS", "bloom_ps.hlsl", Pragma::RHI::PixelFormat::R16G16B16A16_Float);
}

std::unique_ptr<Pragma::RHI::IPipelineState> CreateShadowPipeline(Pragma::RHI::IDevice& device)
{
    static const std::string s_vertexShaderSource = Pragma::Renderer::LoadRendererShaderSource("shadow_vs.hlsl");
    static const std::string s_pixelShaderSource = Pragma::Renderer::LoadRendererShaderSource("shadow_ps.hlsl");

    static constexpr Pragma::RHI::VertexAttributeDesc kVertexAttributes[] =
    {
        { "POSITION", 0, Pragma::RHI::PixelFormat::R32G32B32_Float, 0 },
        { "COLOR", 0, Pragma::RHI::PixelFormat::R32G32B32_Float, 12 },
        { "NORMAL", 0, Pragma::RHI::PixelFormat::R32G32B32_Float, 24 },
        { "TEXCOORD", 0, Pragma::RHI::PixelFormat::R32G32_Float, 36 }
    };

    Pragma::RHI::GraphicsPipelineDesc pipelineDesc;
    pipelineDesc.DebugName = "DirectionalShadowPass";
    pipelineDesc.VertexAttributes = kVertexAttributes;
    pipelineDesc.VertexAttributeCount = static_cast<std::uint32_t>(std::size(kVertexAttributes));
    pipelineDesc.VertexStride = sizeof(VertexPCN);
    pipelineDesc.ColorFormat = Pragma::RHI::PixelFormat::R16G16B16A16_Float;
    pipelineDesc.DepthFormat = Pragma::RHI::PixelFormat::D24_UNorm_S8_UInt;
    pipelineDesc.Rasterizer.FrontCounterClockwise = true;
    pipelineDesc.VertexShader.Stage = Pragma::RHI::ShaderStage::Vertex;
    pipelineDesc.VertexShader.SourceCode = s_vertexShaderSource.c_str();
    pipelineDesc.VertexShader.DebugName = "ShadowVS";
    pipelineDesc.PixelShader.Stage = Pragma::RHI::ShaderStage::Pixel;
    pipelineDesc.PixelShader.SourceCode = s_pixelShaderSource.c_str();
    pipelineDesc.PixelShader.DebugName = "ShadowPS";

    return device.CreateGraphicsPipeline(pipelineDesc);
}

Pragma::Math::Matrix4 BuildDirectionalLightViewProjection(
    const Pragma::Math::Vector3& focusPosition,
    const Pragma::Math::Vector3& lightDirection,
    const ShadowQualitySettings& shadowSettings) noexcept
{
    const Pragma::Math::Vector3 normalizedLightDirection = Pragma::Math::Normalize(lightDirection);
    const Pragma::Math::Vector3 lightEye = focusPosition - normalizedLightDirection * shadowSettings.Distance;
    const Pragma::Math::Vector3 up =
        std::abs(normalizedLightDirection.Y) > 0.95f
            ? Pragma::Math::Vector3{ 0.0f, 0.0f, 1.0f }
            : Pragma::Math::Vector3{ 0.0f, 1.0f, 0.0f };
    const Pragma::Math::Matrix4 lightView = Pragma::Math::LookAtRH(lightEye, focusPosition, up);
    const Pragma::Math::Matrix4 lightProjection = Pragma::Math::OrthographicRH(
        shadowSettings.HalfExtent * 2.0f,
        shadowSettings.HalfExtent * 2.0f,
        shadowSettings.NearPlane,
        shadowSettings.FarPlane);
    return Pragma::Math::Multiply(lightView, lightProjection);
}

Plane NormalizePlane(const Plane& plane) noexcept
{
    const float normalLength = Pragma::Math::Length(plane.Normal);
    if (normalLength <= 0.00001f)
    {
        return plane;
    }

    const float inverseLength = 1.0f / normalLength;
    Plane result = plane;
    result.Normal = result.Normal * inverseLength;
    result.Distance *= inverseLength;
    return result;
}

Plane MakePlaneFromColumnCombination(
    const Pragma::Math::Matrix4& matrix,
    const int columnA,
    const float columnAScale,
    const int columnB = -1,
    const float columnBScale = 0.0f) noexcept
{
    Plane plane{};
    plane.Normal.X =
        matrix.M[0][columnA] * columnAScale +
        (columnB >= 0 ? matrix.M[0][columnB] * columnBScale : 0.0f);
    plane.Normal.Y =
        matrix.M[1][columnA] * columnAScale +
        (columnB >= 0 ? matrix.M[1][columnB] * columnBScale : 0.0f);
    plane.Normal.Z =
        matrix.M[2][columnA] * columnAScale +
        (columnB >= 0 ? matrix.M[2][columnB] * columnBScale : 0.0f);
    plane.Distance =
        matrix.M[3][columnA] * columnAScale +
        (columnB >= 0 ? matrix.M[3][columnB] * columnBScale : 0.0f);
    return NormalizePlane(plane);
}

Frustum ExtractFrustum(const Pragma::Math::Matrix4& worldToClip) noexcept
{
    Frustum frustum{};
    frustum.Left = MakePlaneFromColumnCombination(worldToClip, 3, 1.0f, 0, 1.0f);
    frustum.Right = MakePlaneFromColumnCombination(worldToClip, 3, 1.0f, 0, -1.0f);
    frustum.Bottom = MakePlaneFromColumnCombination(worldToClip, 3, 1.0f, 1, 1.0f);
    frustum.Top = MakePlaneFromColumnCombination(worldToClip, 3, 1.0f, 1, -1.0f);
    frustum.Near = MakePlaneFromColumnCombination(worldToClip, 2, 1.0f);
    frustum.Far = MakePlaneFromColumnCombination(worldToClip, 3, 1.0f, 2, -1.0f);
    return frustum;
}

bool IntersectsPlane(const Plane& plane, const BoundingSphere& sphere) noexcept
{
    return Pragma::Math::Dot(plane.Normal, sphere.Center) + plane.Distance >= -sphere.Radius;
}

bool IsVisibleInFrustum(const Frustum& frustum, const BoundingSphere& sphere) noexcept
{
    return
        IntersectsPlane(frustum.Left, sphere) &&
        IntersectsPlane(frustum.Right, sphere) &&
        IntersectsPlane(frustum.Bottom, sphere) &&
        IntersectsPlane(frustum.Top, sphere) &&
        IntersectsPlane(frustum.Near, sphere) &&
        IntersectsPlane(frustum.Far, sphere);
}

Pragma::Math::Vector3 TransformPoint(const Pragma::Math::Vector3& point, const Pragma::Math::Matrix4& matrix) noexcept
{
    return
    {
        point.X * matrix.M[0][0] + point.Y * matrix.M[1][0] + point.Z * matrix.M[2][0] + matrix.M[3][0],
        point.X * matrix.M[0][1] + point.Y * matrix.M[1][1] + point.Z * matrix.M[2][1] + matrix.M[3][1],
        point.X * matrix.M[0][2] + point.Y * matrix.M[1][2] + point.Z * matrix.M[2][2] + matrix.M[3][2]
    };
}

BoundingSphere BuildWorldBoundingSphere(const Mesh& mesh, const Transform& worldTransform) noexcept
{
    const Pragma::Math::Matrix4 world = ToMatrix(worldTransform);
    const float maxScale = std::max({ std::abs(worldTransform.Scale.X), std::abs(worldTransform.Scale.Y), std::abs(worldTransform.Scale.Z) });
    return
    {
        TransformPoint(mesh.LocalBoundsCenter, world),
        mesh.LocalBoundsRadius * maxScale
    };
}

float ComputeCameraDistance(const BoundingSphere& sphere, const Pragma::Math::Vector3& cameraPosition) noexcept
{
    return std::max(0.0f, Pragma::Math::Length(sphere.Center - cameraPosition) - sphere.Radius);
}

LodLevel ResolveLodLevel(
    const BoundingSphere& sphere,
    const Pragma::Math::Vector3& cameraPosition,
    const Pragma::Core::GraphicsConfig& graphicsConfig) noexcept
{
    if (!graphicsConfig.LodEnabled)
    {
        return LodLevel::High;
    }

    const float radius = std::max(sphere.Radius, 0.75f);
    const float normalizedDistance = ComputeCameraDistance(sphere, cameraPosition) / radius;

    if (normalizedDistance >= graphicsConfig.LodFarNormalizedDistance)
    {
        return LodLevel::Low;
    }

    if (normalizedDistance >= graphicsConfig.LodNearNormalizedDistance)
    {
        return LodLevel::Medium;
    }

    return LodLevel::High;
}

bool ShouldCastShadowForProxy(
    const BoundingSphere& worldBounds,
    const float cameraDistance,
    const LodLevel lodLevel,
    const ShadowQualitySettings& shadowSettings,
    const Pragma::Core::GraphicsConfig& graphicsConfig) noexcept
{
    if (!graphicsConfig.LodEnabled)
    {
        return true;
    }

    if (lodLevel != LodLevel::Low)
    {
        return true;
    }

    if (cameraDistance > shadowSettings.Distance * graphicsConfig.ShadowLowLodDistanceScale)
    {
        return false;
    }

    return worldBounds.Radius >= 0.85f;
}

const Mesh* ResolveMeshForLod(const MeshRendererComponent& meshRenderer, const LodLevel lodLevel) noexcept
{
    switch (lodLevel)
    {
    case LodLevel::Low:
        if (meshRenderer.LowLodMesh != nullptr)
        {
            return meshRenderer.LowLodMesh.get();
        }
        if (meshRenderer.MediumLodMesh != nullptr)
        {
            return meshRenderer.MediumLodMesh.get();
        }
        break;
    case LodLevel::Medium:
        if (meshRenderer.MediumLodMesh != nullptr)
        {
            return meshRenderer.MediumLodMesh.get();
        }
        break;
    case LodLevel::High:
    default:
        break;
    }

    return meshRenderer.Mesh.get();
}

WorldTransformCache BuildWorldTransformCache(const std::vector<SceneObject>& sceneObjects)
{
    WorldTransformCache cache{};
    cache.ObjectIndices.reserve(sceneObjects.size());
    cache.WorldTransforms.resize(sceneObjects.size());
    cache.Computed.resize(sceneObjects.size(), false);

    for (std::size_t objectIndex = 0; objectIndex < sceneObjects.size(); ++objectIndex)
    {
        cache.ObjectIndices.emplace(sceneObjects[objectIndex].Id, objectIndex);
    }

    const auto resolveWorldTransform = [&](auto&& self, const std::size_t objectIndex) -> Transform
    {
        if (cache.Computed[objectIndex])
        {
            return cache.WorldTransforms[objectIndex];
        }

        const SceneObject& object = sceneObjects[objectIndex];
        Transform worldTransform = object.GetTransform();
        if (object.ParentId != InvalidEntityId)
        {
            const auto parentIt = cache.ObjectIndices.find(object.ParentId);
            if (parentIt != cache.ObjectIndices.end() && parentIt->second != objectIndex)
            {
                worldTransform = CombineTransforms(self(self, parentIt->second), worldTransform);
            }
        }

        cache.WorldTransforms[objectIndex] = worldTransform;
        cache.Computed[objectIndex] = true;
        return worldTransform;
    };

    for (std::size_t objectIndex = 0; objectIndex < sceneObjects.size(); ++objectIndex)
    {
        resolveWorldTransform(resolveWorldTransform, objectIndex);
    }

    return cache;
}

const Transform* TryGetCachedWorldTransform(
    const WorldTransformCache& cache,
    const EntityId entityId) noexcept
{
    const auto objectIndexIt = cache.ObjectIndices.find(entityId);
    if (objectIndexIt == cache.ObjectIndices.end())
    {
        return nullptr;
    }

    return &cache.WorldTransforms[objectIndexIt->second];
}
}

RenderSystem::RenderSystem(
    Pragma::RHI::IDevice& device,
    const Pragma::RHI::NativeWindow window,
    const Pragma::RHI::Extent2D extent,
    const Pragma::Core::GraphicsConfig& graphicsConfig)
    : m_device(device)
    , m_graphicsConfig(graphicsConfig)
{
    ApplyGraphicsQualityPreset(m_graphicsConfig);
    SanitizeGraphicsConfig(m_graphicsConfig);
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

    Pragma::RHI::BufferDesc shadowBufferDesc = constantBufferDesc;
    shadowBufferDesc.SizeInBytes = sizeof(ShadowConstants);
    shadowBufferDesc.Stride = sizeof(ShadowConstants);

    Pragma::RHI::BufferDesc skyBufferDesc = constantBufferDesc;
    skyBufferDesc.SizeInBytes = sizeof(SkyConstants);
    skyBufferDesc.Stride = sizeof(SkyConstants);

    Pragma::RHI::BufferDesc bloomBufferDesc = constantBufferDesc;
    bloomBufferDesc.SizeInBytes = sizeof(BloomConstants);
    bloomBufferDesc.Stride = sizeof(BloomConstants);

    Pragma::RHI::BufferDesc tonemapBufferDesc = constantBufferDesc;
    tonemapBufferDesc.SizeInBytes = sizeof(TonemapConstants);
    tonemapBufferDesc.Stride = sizeof(TonemapConstants);

    Pragma::RHI::BufferDesc instanceBufferDesc = constantBufferDesc;
    instanceBufferDesc.SizeInBytes = sizeof(MainInstanceConstants);
    instanceBufferDesc.Stride = sizeof(MainInstanceConstants);

    m_frameConstantBuffer = m_device.CreateBuffer(constantBufferDesc, nullptr);
    m_shadowConstantBuffer = m_device.CreateBuffer(shadowBufferDesc, nullptr);
    m_skyConstantBuffer = m_device.CreateBuffer(skyBufferDesc, nullptr);
    m_bloomConstantBuffer = m_device.CreateBuffer(bloomBufferDesc, nullptr);
    m_tonemapConstantBuffer = m_device.CreateBuffer(tonemapBufferDesc, nullptr);
    m_mainInstanceConstantBuffer = m_device.CreateBuffer(instanceBufferDesc, nullptr);
    m_debugCubeMesh = CreateCubeMesh(m_device);
    m_physicsDebugMaterial = CreateDefaultMaterial(m_device);
    m_lodDebugMaterial = CreateWireframeDebugMaterial(m_device);
    m_physicsDebugMaterial->Parameters.UseAlbedoTexture = 0.0f;
    m_lodDebugMaterial->Parameters.UseAlbedoTexture = 0.0f;
    m_shadowPipeline = CreateShadowPipeline(m_device);
    m_skyPipeline = CreateSkyPipeline(m_device);
    m_bloomPipeline = CreateBloomPipeline(m_device);
    m_tonemapPipeline = CreateTonemapPipeline(m_device);
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

    CreateRenderTargets(extent);
}

void RenderSystem::RenderFrame(
    const Scene& scene,
    const bool showPhysicsOverlay,
    const bool showLodOverlay,
    const std::function<void()>& overlayCallback)
{
    PRAGMA_PROFILE_SCOPE("RenderSystem::RenderFrame");
    PRAGMA_ASSERT(m_commandList != nullptr, "RenderSystem command list is not initialized.");
    PRAGMA_ASSERT(m_swapchain != nullptr, "RenderSystem swapchain is not initialized.");
    PRAGMA_ASSERT(scene.IsInitialized(), "RenderSystem received a scene that has not been initialized.");
    const std::vector<SceneObject>& sceneObjects = scene.GetObjects();
    PRAGMA_ASSERT(!sceneObjects.empty(), "RenderSystem received an empty scene.");
    PRAGMA_ASSERT(scene.GetActiveCameraEntityId() != InvalidEntityId, "RenderSystem received a scene without an active camera.");

    m_lastFrameStatistics = {};
    m_lastFrameStatistics.SceneObjectCount = static_cast<std::uint64_t>(sceneObjects.size());
    const ShadowQualitySettings shadowSettings = ResolveShadowQualitySettings(m_graphicsConfig.ShadowQuality);
    m_lastFrameStatistics.QualityPreset = m_graphicsConfig.QualityPreset;
    m_lastFrameStatistics.ShadingQuality = m_graphicsConfig.ShadingQuality;
    m_lastFrameStatistics.RenderScale = std::clamp(m_graphicsConfig.RenderScale, 0.5f, 1.0f);
    m_lastFrameStatistics.InternalRenderWidth = m_hdrColorTarget != nullptr ? m_hdrColorTarget->GetDesc().Width : m_swapchainDesc.Extent.Width;
    m_lastFrameStatistics.InternalRenderHeight = m_hdrColorTarget != nullptr ? m_hdrColorTarget->GetDesc().Height : m_swapchainDesc.Extent.Height;
    m_lastFrameStatistics.ShadowMapResolution = shadowSettings.MapResolution;
    m_lastFrameStatistics.ShadowDistance = shadowSettings.Distance;
    m_lastFrameStatistics.ShadowHalfExtent = shadowSettings.HalfExtent;
    m_lastFrameStatistics.BloomResolutionScale = std::clamp(m_graphicsConfig.BloomResolutionScale, 0.125f, 0.5f);
    m_lastFrameStatistics.BloomResolutionWidth = m_bloomColorTarget != nullptr ? m_bloomColorTarget->GetDesc().Width : 0u;
    m_lastFrameStatistics.BloomResolutionHeight = m_bloomColorTarget != nullptr ? m_bloomColorTarget->GetDesc().Height : 0u;
    m_lastFrameStatistics.FxaaEnabled = m_graphicsConfig.FxaaEnabled;
    m_lastFrameStatistics.LodEnabled = m_graphicsConfig.LodEnabled;
    m_device.BeginGpuFrameProfile(Pragma::Core::GetCurrentProfileFrameIndex());

    {
        PRAGMA_PROFILE_SCOPE("Render Begin");
        m_commandList->Begin();
        m_commandList->SetRenderTargets(m_hdrColorTarget.get(), m_hdrDepthTarget.get());
        m_commandList->ClearColor({ 0.0f, 0.0f, 0.0f, 1.0f });
        m_commandList->ClearDepth(1.0f);
        m_commandList->SetConstantBuffer(0, *m_frameConstantBuffer);
    }

    DrawSceneToCurrentTargets(scene, m_swapchainDesc.Extent, showPhysicsOverlay, showLodOverlay);

    if (m_graphicsConfig.BloomEnabled && m_bloomColorTarget != nullptr && m_bloomPipeline != nullptr)
    {
        PRAGMA_PROFILE_SCOPE("Bloom Pass");
        m_device.BeginGpuScope("Bloom");
        BloomConstants bloomConstants{};
        bloomConstants.Threshold = m_graphicsConfig.BloomThreshold;
        bloomConstants.Intensity = m_graphicsConfig.BloomIntensity;
        bloomConstants.Quality = m_graphicsConfig.BloomQuality;
        bloomConstants.SourceTexelSize[0] = 1.0f / static_cast<float>(m_hdrColorTarget->GetDesc().Width);
        bloomConstants.SourceTexelSize[1] = 1.0f / static_cast<float>(m_hdrColorTarget->GetDesc().Height);
        m_device.UpdateBuffer(*m_bloomConstantBuffer, &bloomConstants, sizeof(bloomConstants));

        m_commandList->SetRenderTargets(m_bloomColorTarget.get(), nullptr);
        m_commandList->ClearColor({ 0.0f, 0.0f, 0.0f, 1.0f });
        m_commandList->SetGraphicsPipeline(*m_bloomPipeline);
        ++m_lastFrameStatistics.PipelineBinds;
        m_commandList->SetConstantBuffer(0, *m_bloomConstantBuffer);
        m_commandList->SetTexture(0, m_hdrColorTarget.get());
        ++m_lastFrameStatistics.TextureBinds;
        m_commandList->Draw(3, 0);
        ++m_lastFrameStatistics.BloomDrawCalls;
        ++m_lastFrameStatistics.TotalDrawCalls;
        ++m_lastFrameStatistics.BloomTriangles;
        ++m_lastFrameStatistics.TotalTriangles;
        m_commandList->SetTexture(0, nullptr);
        ++m_lastFrameStatistics.TextureBinds;
        m_device.EndGpuScope();
    }

    {
        PRAGMA_PROFILE_SCOPE("Tonemap Pass");
        m_device.BeginGpuScope("Tonemap");
        TonemapConstants tonemapConstants{};
        tonemapConstants.Exposure = m_graphicsConfig.Exposure;
        tonemapConstants.BloomIntensity = m_graphicsConfig.BloomEnabled ? m_graphicsConfig.BloomIntensity : 0.0f;
        tonemapConstants.FxaaEnabled = m_graphicsConfig.FxaaEnabled ? 1.0f : 0.0f;
        tonemapConstants.FxaaSubpixel = m_graphicsConfig.FxaaSubpixel;
        tonemapConstants.FxaaEdgeThreshold = m_graphicsConfig.FxaaEdgeThreshold;
        tonemapConstants.FxaaEdgeThresholdMin = m_graphicsConfig.FxaaEdgeThresholdMin;
        tonemapConstants.SourceTexelSize[0] = 1.0f / static_cast<float>(m_hdrColorTarget->GetDesc().Width);
        tonemapConstants.SourceTexelSize[1] = 1.0f / static_cast<float>(m_hdrColorTarget->GetDesc().Height);
        m_device.UpdateBuffer(*m_tonemapConstantBuffer, &tonemapConstants, sizeof(tonemapConstants));

        m_commandList->SetRenderTargets(nullptr, nullptr);
        m_commandList->SetGraphicsPipeline(*m_tonemapPipeline);
        ++m_lastFrameStatistics.PipelineBinds;
        m_commandList->SetConstantBuffer(0, *m_tonemapConstantBuffer);
        m_commandList->SetTexture(0, m_hdrColorTarget.get());
        ++m_lastFrameStatistics.TextureBinds;
        m_commandList->SetTexture(1, m_graphicsConfig.BloomEnabled ? m_bloomColorTarget.get() : nullptr);
        ++m_lastFrameStatistics.TextureBinds;
        m_commandList->Draw(3, 0);
        ++m_lastFrameStatistics.TonemapDrawCalls;
        ++m_lastFrameStatistics.TotalDrawCalls;
        m_lastFrameStatistics.TonemapTriangles += 1;
        m_lastFrameStatistics.TotalTriangles += 1;
        m_commandList->SetTexture(0, nullptr);
        ++m_lastFrameStatistics.TextureBinds;
        m_commandList->SetTexture(4, nullptr);
        ++m_lastFrameStatistics.TextureBinds;
        m_device.EndGpuScope();
    }

    static bool hasLoggedFirstFrame = false;
    if (!hasLoggedFirstFrame)
    {
        hasLoggedFirstFrame = true;
        Pragma::Core::Log(
            Pragma::Core::LogCategory::Renderer,
            Pragma::Core::LogLevel::Info,
            "First render frame: " + std::to_string(sceneObjects.size()) + " objects.");
        Pragma::Core::Log(
            Pragma::Core::LogCategory::Renderer,
            Pragma::Core::LogLevel::Info,
            "Render stats baseline: mesh=" +
            std::to_string(m_lastFrameStatistics.MeshRendererCount) +
            ", worldCache=" + std::to_string(m_lastFrameStatistics.CachedWorldTransformCount) +
            ", proxies=" + std::to_string(m_lastFrameStatistics.RenderProxyCount) +
            ", visible=" + std::to_string(m_lastFrameStatistics.MainPassVisibleMeshCount) +
            ", culled=" + std::to_string(m_lastFrameStatistics.MainPassCulledMeshCount) +
            ", shadowVisible=" + std::to_string(m_lastFrameStatistics.ShadowPassVisibleMeshCount) +
            ", shadowLodSkipped=" + std::to_string(m_lastFrameStatistics.ShadowPassLodSkippedMeshCount) +
            ", lodHigh=" + std::to_string(m_lastFrameStatistics.HighLodMeshCount) +
            ", lodMedium=" + std::to_string(m_lastFrameStatistics.MediumLodMeshCount) +
            ", lodLow=" + std::to_string(m_lastFrameStatistics.LowLodMeshCount) +
            ", draws=" + std::to_string(m_lastFrameStatistics.TotalDrawCalls) +
            ", skyDraws=" + std::to_string(m_lastFrameStatistics.SkyDrawCalls) +
            ", bloomDraws=" + std::to_string(m_lastFrameStatistics.BloomDrawCalls) +
            ", instancedDraws=" + std::to_string(m_lastFrameStatistics.InstancedDrawCalls) +
            ", instancedInstances=" + std::to_string(m_lastFrameStatistics.InstancedInstanceCount) +
            ", quality=" + std::string(Pragma::Core::ToString(m_lastFrameStatistics.QualityPreset)) +
            ", shading=" + std::string(Pragma::Core::ToString(m_lastFrameStatistics.ShadingQuality)) +
            ", renderScale=" + std::to_string(m_lastFrameStatistics.RenderScale) +
            ", hdrRes=" + std::to_string(m_lastFrameStatistics.InternalRenderWidth) + "x" + std::to_string(m_lastFrameStatistics.InternalRenderHeight) +
            ", shadowQuality=" + std::string(Pragma::Core::ToString(m_graphicsConfig.ShadowQuality)) +
            ", lod=" + std::string(m_lastFrameStatistics.LodEnabled ? "on" : "off") +
            ", shadowRes=" + std::to_string(m_lastFrameStatistics.ShadowMapResolution) +
            ", shadowDistance=" + std::to_string(m_lastFrameStatistics.ShadowDistance) +
            ", bloomScale=" + std::to_string(m_lastFrameStatistics.BloomResolutionScale) +
            ", bloomRes=" + std::to_string(m_lastFrameStatistics.BloomResolutionWidth) + "x" + std::to_string(m_lastFrameStatistics.BloomResolutionHeight) +
            ", fxaa=" + std::string(m_lastFrameStatistics.FxaaEnabled ? "on" : "off") +
            ", triangles=" + std::to_string(m_lastFrameStatistics.TotalTriangles) +
            ", psoBinds=" + std::to_string(m_lastFrameStatistics.PipelineBinds) +
            ", textureBinds=" + std::to_string(m_lastFrameStatistics.TextureBinds) + ".");
    }

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
        m_device.EndGpuFrameProfile();
        m_swapchain->Present();
    }
}

void RenderSystem::DrawSceneToCurrentTargets(
    const Scene& scene,
    const Pragma::RHI::Extent2D extent,
    const bool showPhysicsOverlay,
    const bool showLodOverlay)
{
    PRAGMA_ASSERT(scene.IsInitialized(), "RenderSystem received a scene that has not been initialized.");
    const std::vector<SceneObject>& sceneObjects = scene.GetObjects();
    PRAGMA_ASSERT(!sceneObjects.empty(), "RenderSystem received an empty scene.");
    PRAGMA_ASSERT(scene.GetActiveCameraEntityId() != InvalidEntityId, "RenderSystem received a scene without an active camera.");
    const WorldTransformCache worldTransformCache = BuildWorldTransformCache(sceneObjects);
    m_lastFrameStatistics.CachedWorldTransformCount = static_cast<std::uint64_t>(worldTransformCache.WorldTransforms.size());

    const SceneObject* activeCameraObject = scene.GetActiveCameraObject();
    PRAGMA_ASSERT(activeCameraObject != nullptr, "RenderSystem failed to resolve the active camera object.");
    PRAGMA_ASSERT(activeCameraObject->HasCamera(), "Active camera object is missing CameraComponent.");
    const CameraComponent* activeCameraComponent = activeCameraObject->GetCamera();
    PRAGMA_ASSERT(activeCameraComponent != nullptr, "Active camera object returned a null CameraComponent.");
    const Transform* activeCameraWorldTransformPtr = TryGetCachedWorldTransform(worldTransformCache, activeCameraObject->Id);
    PRAGMA_ASSERT(activeCameraWorldTransformPtr != nullptr, "RenderSystem failed to resolve cached active camera transform.");
    const Transform& activeCameraWorldTransform = *activeCameraWorldTransformPtr;

    const float aspectRatio = static_cast<float>(extent.Width) / static_cast<float>(extent.Height);
    Camera activeCamera;
    activeCamera.SetPerspective(
        activeCameraComponent->FieldOfViewRadians,
        aspectRatio,
        activeCameraComponent->NearPlane,
        activeCameraComponent->FarPlane);
    activeCamera.SetPose(
        activeCameraWorldTransform.Position,
        activeCameraWorldTransform.RotationRadians.Y,
        activeCameraComponent->PitchRadians);

    const Pragma::Math::Matrix4 viewProjection = activeCamera.GetViewProjection();
    const Frustum mainFrustum = ExtractFrustum(viewProjection);
    FrameConstants lightingConstants{};
    Pragma::Math::Matrix4 lightViewProjection = Pragma::Math::Identity();
    bool hasDirectionalLight = false;
    const ShadowQualitySettings shadowSettings = ResolveShadowQualitySettings(m_graphicsConfig.ShadowQuality);

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
        lightViewProjection = BuildDirectionalLightViewProjection(
            activeCameraWorldTransform.Position,
            { light->Direction[0], light->Direction[1], light->Direction[2] },
            shadowSettings);
        hasDirectionalLight = true;
        break;
    }
    lightingConstants.CameraPosition[0] = activeCameraWorldTransform.Position.X;
    lightingConstants.CameraPosition[1] = activeCameraWorldTransform.Position.Y;
    lightingConstants.CameraPosition[2] = activeCameraWorldTransform.Position.Z;
    lightingConstants.AmbientStrength = m_graphicsConfig.AmbientStrength;
    lightingConstants.EnvironmentDiffuseStrength = m_graphicsConfig.EnvironmentDiffuseStrength;
    lightingConstants.EnvironmentSpecularStrength = m_graphicsConfig.EnvironmentSpecularStrength;
    lightingConstants.FogStartDistance = m_graphicsConfig.FogStartDistance;
    lightingConstants.FogDensity = m_graphicsConfig.FogDensity;
    lightingConstants.FogHeightFalloff = m_graphicsConfig.FogHeightFalloff;
    lightingConstants.FogMaxOpacity = m_graphicsConfig.FogMaxOpacity;
    lightingConstants.ShadowMapTexelSize[0] = m_shadowColorTarget != nullptr ? (1.0f / static_cast<float>(m_shadowColorTarget->GetDesc().Width)) : 0.0f;
    lightingConstants.ShadowMapTexelSize[1] = m_shadowColorTarget != nullptr ? (1.0f / static_cast<float>(m_shadowColorTarget->GetDesc().Height)) : 0.0f;
    lightingConstants.ShadowBias = shadowSettings.Bias;
    lightingConstants.ShadowStrength = hasDirectionalLight ? 1.0f : 0.0f;
    lightingConstants.ShadowFilterQuality = m_graphicsConfig.ShadowFilterQuality;
    lightingConstants.ShadingQuality = ToShaderQualityValue(m_graphicsConfig.ShadingQuality);
    const Frustum shadowFrustum = hasDirectionalLight ? ExtractFrustum(lightViewProjection) : Frustum{};
    std::vector<RenderProxy> renderProxies;
    renderProxies.reserve(sceneObjects.size());

    for (std::size_t objectIndex = 0; objectIndex < sceneObjects.size(); ++objectIndex)
    {
        const SceneObject& object = sceneObjects[objectIndex];
        const MeshRendererComponent* meshRenderer = object.GetMeshRenderer();
        if (meshRenderer == nullptr ||
            meshRenderer->Mesh == nullptr ||
            meshRenderer->Material == nullptr ||
            meshRenderer->Material->Pipeline == nullptr)
        {
            continue;
        }

        ++m_lastFrameStatistics.MeshRendererCount;

        const Transform& worldTransform = worldTransformCache.WorldTransforms[objectIndex];
        const BoundingSphere worldBounds = BuildWorldBoundingSphere(*meshRenderer->Mesh, worldTransform);
        const float cameraDistance = ComputeCameraDistance(worldBounds, activeCameraWorldTransform.Position);
        const LodLevel lodLevel = ResolveLodLevel(worldBounds, activeCameraWorldTransform.Position, m_graphicsConfig);
        const Mesh* lodMesh = ResolveMeshForLod(*meshRenderer, lodLevel);
        if (lodMesh == nullptr)
        {
            continue;
        }

        switch (lodLevel)
        {
        case LodLevel::High:
            ++m_lastFrameStatistics.HighLodMeshCount;
            break;
        case LodLevel::Medium:
            ++m_lastFrameStatistics.MediumLodMeshCount;
            break;
        case LodLevel::Low:
            ++m_lastFrameStatistics.LowLodMeshCount;
            break;
        }

        const bool mainVisible = IsVisibleInFrustum(mainFrustum, worldBounds);
        if (mainVisible)
        {
            ++m_lastFrameStatistics.MainPassVisibleMeshCount;
        }
        else
        {
            ++m_lastFrameStatistics.MainPassCulledMeshCount;
        }

        bool castsShadow = false;
        if (hasDirectionalLight)
        {
            if (!IsVisibleInFrustum(shadowFrustum, worldBounds))
            {
                ++m_lastFrameStatistics.ShadowPassCulledMeshCount;
            }
            else if (ShouldCastShadowForProxy(
                worldBounds,
                cameraDistance,
                lodLevel,
                shadowSettings,
                m_graphicsConfig))
            {
                castsShadow = true;
                ++m_lastFrameStatistics.ShadowPassVisibleMeshCount;
            }
            else
            {
                ++m_lastFrameStatistics.ShadowPassLodSkippedMeshCount;
            }
        }

        if (!mainVisible && !castsShadow)
        {
            continue;
        }

        const Pragma::Math::Matrix4 world = ToMatrix(worldTransform);
        Transform worldRotationOnly = worldTransform;
        worldRotationOnly.Scale = { 1.0f, 1.0f, 1.0f };

        RenderProxy proxy{};
        proxy.Mesh = lodMesh;
        proxy.Material = meshRenderer->Material.get();
        proxy.WorldBounds = worldBounds;
        proxy.World = world;
        proxy.WorldNoScale = ToMatrix(worldRotationOnly);
        proxy.WorldViewProjection = Pragma::Math::Multiply(world, viewProjection);
        proxy.WorldLightClip = castsShadow ? Pragma::Math::Multiply(world, lightViewProjection) : Pragma::Math::Matrix4{};
        proxy.CameraDistance = cameraDistance;
        proxy.Level = lodLevel;
        proxy.MainVisible = mainVisible;
        proxy.CastsShadow = castsShadow;
        renderProxies.push_back(proxy);
    }

    m_lastFrameStatistics.RenderProxyCount = static_cast<std::uint64_t>(renderProxies.size());

    if (m_skyPipeline != nullptr && m_skyConstantBuffer != nullptr)
    {
        PRAGMA_PROFILE_SCOPE("Draw Sky");
        m_device.BeginGpuScope("Sky");
        const float cosPitch = std::cos(activeCamera.GetPitchRadians());
        const Pragma::Math::Vector3 forward
        {
            std::sin(activeCamera.GetYawRadians()) * cosPitch,
            std::sin(activeCamera.GetPitchRadians()),
            std::cos(activeCamera.GetYawRadians()) * cosPitch
        };
        const Pragma::Math::Vector3 right = Pragma::Math::Normalize(Pragma::Math::Cross({ 0.0f, 1.0f, 0.0f }, forward));
        const Pragma::Math::Vector3 up = Pragma::Math::Normalize(Pragma::Math::Cross(forward, right));
        const Pragma::Math::Vector3 sunDirection = hasDirectionalLight
            ? Pragma::Math::Normalize(Pragma::Math::Vector3{ -lightingConstants.LightDirection[0], -lightingConstants.LightDirection[1], -lightingConstants.LightDirection[2] })
            : Pragma::Math::Vector3{ 0.2f, 0.8f, 0.4f };

        SkyConstants skyConstants{};
        skyConstants.CameraForward[0] = forward.X;
        skyConstants.CameraForward[1] = forward.Y;
        skyConstants.CameraForward[2] = forward.Z;
        skyConstants.TanHalfFovY = std::tan(activeCameraComponent->FieldOfViewRadians * 0.5f);
        skyConstants.CameraRight[0] = right.X;
        skyConstants.CameraRight[1] = right.Y;
        skyConstants.CameraRight[2] = right.Z;
        skyConstants.AspectRatio = aspectRatio;
        skyConstants.CameraUp[0] = up.X;
        skyConstants.CameraUp[1] = up.Y;
        skyConstants.CameraUp[2] = up.Z;
        skyConstants.SunIntensity = lightingConstants.LightIntensity;
        skyConstants.SunDirection[0] = sunDirection.X;
        skyConstants.SunDirection[1] = sunDirection.Y;
        skyConstants.SunDirection[2] = sunDirection.Z;
        skyConstants.ZenithColor[0] = lightingConstants.SkyZenithColor[0];
        skyConstants.ZenithColor[1] = lightingConstants.SkyZenithColor[1];
        skyConstants.ZenithColor[2] = lightingConstants.SkyZenithColor[2];
        skyConstants.HorizonColor[0] = lightingConstants.SkyHorizonColor[0];
        skyConstants.HorizonColor[1] = lightingConstants.SkyHorizonColor[1];
        skyConstants.HorizonColor[2] = lightingConstants.SkyHorizonColor[2];
        skyConstants.GroundColor[0] = lightingConstants.SkyGroundColor[0];
        skyConstants.GroundColor[1] = lightingConstants.SkyGroundColor[1];
        skyConstants.GroundColor[2] = lightingConstants.SkyGroundColor[2];
        skyConstants.AtmosphereDensity = lightingConstants.SkyAtmosphereDensity;

        m_device.UpdateBuffer(*m_skyConstantBuffer, &skyConstants, sizeof(skyConstants));
        m_commandList->SetGraphicsPipeline(*m_skyPipeline);
        ++m_lastFrameStatistics.PipelineBinds;
        m_commandList->SetConstantBuffer(0, *m_skyConstantBuffer);
        m_commandList->Draw(3, 0);
        ++m_lastFrameStatistics.SkyDrawCalls;
        ++m_lastFrameStatistics.TotalDrawCalls;
        ++m_lastFrameStatistics.SkyTriangles;
        ++m_lastFrameStatistics.TotalTriangles;
        m_commandList->SetConstantBuffer(0, *m_frameConstantBuffer);
        m_device.EndGpuScope();
    }

    if (hasDirectionalLight && m_shadowColorTarget != nullptr && m_shadowDepthTarget != nullptr && m_shadowPipeline != nullptr)
    {
        PRAGMA_PROFILE_SCOPE("Draw Shadow Map");
        m_device.BeginGpuScope("Shadow");
        m_commandList->SetTexture(1, nullptr);
        ++m_lastFrameStatistics.TextureBinds;
        m_commandList->SetRenderTargets(m_shadowColorTarget.get(), m_shadowDepthTarget.get());
        m_commandList->ClearColor({ 1.0f, 1.0f, 1.0f, 1.0f });
        m_commandList->ClearDepth(1.0f);
        m_commandList->SetGraphicsPipeline(*m_shadowPipeline);
        ++m_lastFrameStatistics.PipelineBinds;
        m_commandList->SetConstantBuffer(0, *m_shadowConstantBuffer);
        std::vector<ShadowRenderItem> shadowRenderItems;
        shadowRenderItems.reserve(renderProxies.size());

        for (const RenderProxy& proxy : renderProxies)
        {
            if (!proxy.CastsShadow)
            {
                continue;
            }

            shadowRenderItems.push_back(
                {
                    proxy.Mesh,
                    proxy.WorldLightClip
                });
        }

        std::sort(
            shadowRenderItems.begin(),
            shadowRenderItems.end(),
            [](const ShadowRenderItem& lhs, const ShadowRenderItem& rhs)
            {
                if (lhs.Mesh != rhs.Mesh)
                {
                    return lhs.Mesh < rhs.Mesh;
                }

                return lhs.Mesh->IndexCount < rhs.Mesh->IndexCount;
            });

        const Pragma::RHI::IBuffer* lastVertexBuffer = nullptr;
        const Pragma::RHI::IBuffer* lastIndexBuffer = nullptr;

        for (std::size_t itemIndex = 0; itemIndex < shadowRenderItems.size();)
        {
            const Mesh* batchMesh = shadowRenderItems[itemIndex].Mesh;
            std::size_t batchCount = 1;
            while (itemIndex + batchCount < shadowRenderItems.size() &&
                shadowRenderItems[itemIndex + batchCount].Mesh == batchMesh &&
                batchCount < kMaxBatchedInstances)
            {
                ++batchCount;
            }

            ShadowConstants shadowConstants{};
            for (std::size_t instanceIndex = 0; instanceIndex < batchCount; ++instanceIndex)
            {
                shadowConstants.WorldLightClip[instanceIndex] = shadowRenderItems[itemIndex + instanceIndex].WorldLightClip;
            }
            m_device.UpdateBuffer(*m_shadowConstantBuffer, &shadowConstants, sizeof(shadowConstants));

            if (lastVertexBuffer != batchMesh->VertexBuffer.get())
            {
                m_commandList->SetVertexBuffer(*batchMesh->VertexBuffer, 0);
                lastVertexBuffer = batchMesh->VertexBuffer.get();
                ++m_lastFrameStatistics.VertexBufferBinds;
            }

            if (lastIndexBuffer != batchMesh->IndexBuffer.get())
            {
                m_commandList->SetIndexBuffer(*batchMesh->IndexBuffer, batchMesh->IndexFormat, 0);
                lastIndexBuffer = batchMesh->IndexBuffer.get();
                ++m_lastFrameStatistics.IndexBufferBinds;
            }

            if (batchCount > 1)
            {
                m_commandList->DrawIndexedInstanced(batchMesh->IndexCount, static_cast<std::uint32_t>(batchCount), 0, 0, 0);
                ++m_lastFrameStatistics.InstancedDrawCalls;
                m_lastFrameStatistics.InstancedInstanceCount += static_cast<std::uint64_t>(batchCount);
            }
            else
            {
                m_commandList->DrawIndexed(batchMesh->IndexCount, 0, 0);
            }

            ++m_lastFrameStatistics.ShadowPassDrawCalls;
            ++m_lastFrameStatistics.TotalDrawCalls;
            const std::uint64_t triangles = static_cast<std::uint64_t>(batchMesh->IndexCount / 3) * static_cast<std::uint64_t>(batchCount);
            m_lastFrameStatistics.ShadowPassTriangles += triangles;
            m_lastFrameStatistics.TotalTriangles += triangles;
            itemIndex += batchCount;
        }

        m_commandList->SetRenderTargets(m_hdrColorTarget.get(), m_hdrDepthTarget.get());
        m_commandList->SetConstantBuffer(0, *m_frameConstantBuffer);
        m_device.EndGpuScope();
    }

    PRAGMA_PROFILE_SCOPE("Draw Scene");
    m_device.BeginGpuScope("Main");
    m_commandList->SetTexture(4, hasDirectionalLight ? m_shadowColorTarget.get() : nullptr);
    std::vector<MainRenderItem> mainRenderItems;
    mainRenderItems.reserve(renderProxies.size());
    for (const RenderProxy& proxy : renderProxies)
    {
        if (!proxy.MainVisible)
        {
            continue;
        }

        mainRenderItems.push_back(
            {
                proxy.Mesh,
                proxy.Material,
                proxy.World,
                proxy.WorldNoScale,
                proxy.WorldViewProjection,
                proxy.WorldLightClip
            });
    }

    std::sort(
        mainRenderItems.begin(),
        mainRenderItems.end(),
        [](const MainRenderItem& lhs, const MainRenderItem& rhs)
        {
            if (lhs.Material->Pipeline.get() != rhs.Material->Pipeline.get())
            {
                return lhs.Material->Pipeline.get() < rhs.Material->Pipeline.get();
            }
            if (lhs.Material != rhs.Material)
            {
                return lhs.Material < rhs.Material;
            }
            if (lhs.Material->AlbedoTexture.get() != rhs.Material->AlbedoTexture.get())
            {
                return lhs.Material->AlbedoTexture.get() < rhs.Material->AlbedoTexture.get();
            }
            if (lhs.Material->NormalTexture.get() != rhs.Material->NormalTexture.get())
            {
                return lhs.Material->NormalTexture.get() < rhs.Material->NormalTexture.get();
            }
            if (lhs.Material->OrmTexture.get() != rhs.Material->OrmTexture.get())
            {
                return lhs.Material->OrmTexture.get() < rhs.Material->OrmTexture.get();
            }
            if (lhs.Material->EmissiveTexture.get() != rhs.Material->EmissiveTexture.get())
            {
                return lhs.Material->EmissiveTexture.get() < rhs.Material->EmissiveTexture.get();
            }
            if (lhs.Mesh != rhs.Mesh)
            {
                return lhs.Mesh < rhs.Mesh;
            }

            return lhs.Mesh->IndexCount < rhs.Mesh->IndexCount;
        });

    const Pragma::RHI::IPipelineState* lastPipeline = nullptr;
    const Pragma::RHI::IBuffer* lastVertexBuffer = nullptr;
    const Pragma::RHI::IBuffer* lastIndexBuffer = nullptr;
    const Pragma::RHI::IBuffer* lastMaterialBuffer = nullptr;
    const Pragma::RHI::ITexture* lastAlbedoTexture = nullptr;
    const Pragma::RHI::ITexture* lastNormalTexture = nullptr;
    const Pragma::RHI::ITexture* lastOrmTexture = nullptr;
    const Pragma::RHI::ITexture* lastEmissiveTexture = nullptr;
    m_device.UpdateBuffer(*m_frameConstantBuffer, &lightingConstants, sizeof(lightingConstants));
    m_commandList->SetConstantBuffer(2, *m_mainInstanceConstantBuffer);

    for (std::size_t itemIndex = 0; itemIndex < mainRenderItems.size();)
    {
        const MainRenderItem& firstItem = mainRenderItems[itemIndex];
        std::size_t batchCount = 1;
        while (itemIndex + batchCount < mainRenderItems.size() &&
            mainRenderItems[itemIndex + batchCount].Material->Pipeline.get() == firstItem.Material->Pipeline.get() &&
            mainRenderItems[itemIndex + batchCount].Material == firstItem.Material &&
            mainRenderItems[itemIndex + batchCount].Material->AlbedoTexture.get() == firstItem.Material->AlbedoTexture.get() &&
            mainRenderItems[itemIndex + batchCount].Material->NormalTexture.get() == firstItem.Material->NormalTexture.get() &&
            mainRenderItems[itemIndex + batchCount].Material->OrmTexture.get() == firstItem.Material->OrmTexture.get() &&
            mainRenderItems[itemIndex + batchCount].Material->EmissiveTexture.get() == firstItem.Material->EmissiveTexture.get() &&
            mainRenderItems[itemIndex + batchCount].Mesh == firstItem.Mesh &&
            batchCount < kMaxBatchedInstances)
        {
            ++batchCount;
        }

        MainInstanceConstants instanceConstants{};
        for (std::size_t instanceIndex = 0; instanceIndex < batchCount; ++instanceIndex)
        {
            const MainRenderItem& renderItem = mainRenderItems[itemIndex + instanceIndex];
            instanceConstants.WorldViewProjection[instanceIndex] = renderItem.WorldViewProjection;
            instanceConstants.World[instanceIndex] = renderItem.World;
            instanceConstants.WorldNoScale[instanceIndex] = renderItem.WorldNoScale;
            instanceConstants.WorldLightClip[instanceIndex] = renderItem.WorldLightClip;
        }
        m_device.UpdateBuffer(*m_mainInstanceConstantBuffer, &instanceConstants, sizeof(instanceConstants));

        if (lastPipeline != firstItem.Material->Pipeline.get())
        {
            m_commandList->SetGraphicsPipeline(*firstItem.Material->Pipeline);
            lastPipeline = firstItem.Material->Pipeline.get();
            ++m_lastFrameStatistics.PipelineBinds;
        }

        if (lastVertexBuffer != firstItem.Mesh->VertexBuffer.get())
        {
            m_commandList->SetVertexBuffer(*firstItem.Mesh->VertexBuffer, 0);
            lastVertexBuffer = firstItem.Mesh->VertexBuffer.get();
            ++m_lastFrameStatistics.VertexBufferBinds;
        }

        if (lastIndexBuffer != firstItem.Mesh->IndexBuffer.get())
        {
            m_commandList->SetIndexBuffer(*firstItem.Mesh->IndexBuffer, firstItem.Mesh->IndexFormat, 0);
            lastIndexBuffer = firstItem.Mesh->IndexBuffer.get();
            ++m_lastFrameStatistics.IndexBufferBinds;
        }

        if (lastMaterialBuffer != firstItem.Material->ParametersBuffer.get())
        {
            m_commandList->SetConstantBuffer(1, *firstItem.Material->ParametersBuffer);
            lastMaterialBuffer = firstItem.Material->ParametersBuffer.get();
            ++m_lastFrameStatistics.MaterialBufferBinds;
        }

        if (lastAlbedoTexture != firstItem.Material->AlbedoTexture.get())
        {
            m_commandList->SetTexture(0, firstItem.Material->AlbedoTexture.get());
            lastAlbedoTexture = firstItem.Material->AlbedoTexture.get();
            ++m_lastFrameStatistics.TextureBinds;
        }
        if (lastNormalTexture != firstItem.Material->NormalTexture.get())
        {
            m_commandList->SetTexture(1, firstItem.Material->NormalTexture.get());
            lastNormalTexture = firstItem.Material->NormalTexture.get();
            ++m_lastFrameStatistics.TextureBinds;
        }
        if (lastOrmTexture != firstItem.Material->OrmTexture.get())
        {
            m_commandList->SetTexture(2, firstItem.Material->OrmTexture.get());
            lastOrmTexture = firstItem.Material->OrmTexture.get();
            ++m_lastFrameStatistics.TextureBinds;
        }
        if (lastEmissiveTexture != firstItem.Material->EmissiveTexture.get())
        {
            m_commandList->SetTexture(3, firstItem.Material->EmissiveTexture.get());
            lastEmissiveTexture = firstItem.Material->EmissiveTexture.get();
            ++m_lastFrameStatistics.TextureBinds;
        }

        if (batchCount > 1)
        {
            m_commandList->DrawIndexedInstanced(firstItem.Mesh->IndexCount, static_cast<std::uint32_t>(batchCount), 0, 0, 0);
            ++m_lastFrameStatistics.InstancedDrawCalls;
            m_lastFrameStatistics.InstancedInstanceCount += static_cast<std::uint64_t>(batchCount);
        }
        else
        {
            m_commandList->DrawIndexed(firstItem.Mesh->IndexCount, 0, 0);
        }

        ++m_lastFrameStatistics.MainPassDrawCalls;
        ++m_lastFrameStatistics.TotalDrawCalls;
        const std::uint64_t triangles = static_cast<std::uint64_t>(firstItem.Mesh->IndexCount / 3) * static_cast<std::uint64_t>(batchCount);
        m_lastFrameStatistics.MainPassTriangles += triangles;
        m_lastFrameStatistics.TotalTriangles += triangles;
        itemIndex += batchCount;
    }
    m_device.EndGpuScope();

    if (showPhysicsOverlay && m_debugCubeMesh != nullptr && m_physicsDebugMaterial != nullptr)
    {
        PRAGMA_PROFILE_SCOPE("Draw Physics Overlay");
        m_device.BeginGpuScope("PhysicsOverlay");
        for (std::size_t objectIndex = 0; objectIndex < sceneObjects.size(); ++objectIndex)
        {
            const SceneObject& object = sceneObjects[objectIndex];
            const auto* boxCollider = object.GetBoxCollider();
            const auto* rigidBody = object.GetRigidBody();
            if (boxCollider == nullptr && rigidBody == nullptr)
            {
                continue;
            }

            ++m_lastFrameStatistics.PhysicsOverlayObjectCount;

            const bool incompletePhysicsSetup = boxCollider == nullptr || rigidBody == nullptr;
            const bool invalidCollider =
                boxCollider != nullptr &&
                (boxCollider->HalfExtent.X <= 0.0f || boxCollider->HalfExtent.Y <= 0.0f || boxCollider->HalfExtent.Z <= 0.0f);

            Transform worldTransform = worldTransformCache.WorldTransforms[objectIndex];
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

            const Pragma::Math::Matrix4 world = ToMatrix(worldTransform);
            Transform worldRotationOnly = worldTransform;
            worldRotationOnly.Scale = { 1.0f, 1.0f, 1.0f };
            MainInstanceConstants instanceConstants{};
            instanceConstants.WorldViewProjection[0] = Pragma::Math::Multiply(world, viewProjection);
            instanceConstants.World[0] = world;
            instanceConstants.WorldNoScale[0] = ToMatrix(worldRotationOnly);
            instanceConstants.WorldLightClip[0] = Pragma::Math::Multiply(world, lightViewProjection);
            m_device.UpdateBuffer(*m_mainInstanceConstantBuffer, &instanceConstants, sizeof(instanceConstants));

            m_commandList->SetGraphicsPipeline(*m_physicsDebugMaterial->Pipeline);
            ++m_lastFrameStatistics.PipelineBinds;
            m_commandList->SetVertexBuffer(*m_debugCubeMesh->VertexBuffer, 0);
            ++m_lastFrameStatistics.VertexBufferBinds;
            m_commandList->SetIndexBuffer(*m_debugCubeMesh->IndexBuffer, m_debugCubeMesh->IndexFormat, 0);
            ++m_lastFrameStatistics.IndexBufferBinds;
            m_commandList->SetConstantBuffer(1, *m_physicsDebugMaterial->ParametersBuffer);
            ++m_lastFrameStatistics.MaterialBufferBinds;
            m_commandList->SetConstantBuffer(2, *m_mainInstanceConstantBuffer);
            m_commandList->SetTexture(0, nullptr);
            ++m_lastFrameStatistics.TextureBinds;
            m_commandList->DrawIndexed(m_debugCubeMesh->IndexCount, 0, 0);
            ++m_lastFrameStatistics.PhysicsOverlayDrawCalls;
            ++m_lastFrameStatistics.TotalDrawCalls;
            const std::uint64_t triangles = static_cast<std::uint64_t>(m_debugCubeMesh->IndexCount / 3);
            m_lastFrameStatistics.PhysicsOverlayTriangles += triangles;
            m_lastFrameStatistics.TotalTriangles += triangles;
        }
        m_device.EndGpuScope();
    }

    if (showLodOverlay && m_debugCubeMesh != nullptr && m_lodDebugMaterial != nullptr)
    {
        PRAGMA_PROFILE_SCOPE("Draw LOD Overlay");
        m_device.BeginGpuScope("LodOverlay");
        for (const RenderProxy& proxy : renderProxies)
        {
            if (!proxy.MainVisible)
            {
                continue;
            }

            ++m_lastFrameStatistics.LodOverlayObjectCount;

            switch (proxy.Level)
            {
            case LodLevel::High:
                m_lodDebugMaterial->Parameters.BaseColor[0] = 0.25f;
                m_lodDebugMaterial->Parameters.BaseColor[1] = 1.0f;
                m_lodDebugMaterial->Parameters.BaseColor[2] = 0.35f;
                break;
            case LodLevel::Medium:
                m_lodDebugMaterial->Parameters.BaseColor[0] = 1.0f;
                m_lodDebugMaterial->Parameters.BaseColor[1] = 0.82f;
                m_lodDebugMaterial->Parameters.BaseColor[2] = 0.2f;
                break;
            case LodLevel::Low:
            default:
                m_lodDebugMaterial->Parameters.BaseColor[0] = 1.0f;
                m_lodDebugMaterial->Parameters.BaseColor[1] = 0.3f;
                m_lodDebugMaterial->Parameters.BaseColor[2] = 0.3f;
                break;
            }
            m_lodDebugMaterial->Parameters.BaseColor[3] = 1.0f;
            m_lodDebugMaterial->Parameters.Roughness = 0.15f;
            m_lodDebugMaterial->Parameters.Metallic = 0.0f;
            m_lodDebugMaterial->Parameters.AmbientOcclusion = 1.0f;
            m_lodDebugMaterial->Parameters.UseAlbedoTexture = 0.0f;
            m_lodDebugMaterial->Parameters.EmissiveColor[0] = m_lodDebugMaterial->Parameters.BaseColor[0];
            m_lodDebugMaterial->Parameters.EmissiveColor[1] = m_lodDebugMaterial->Parameters.BaseColor[1];
            m_lodDebugMaterial->Parameters.EmissiveColor[2] = m_lodDebugMaterial->Parameters.BaseColor[2];
            m_lodDebugMaterial->Parameters.EmissiveIntensity = 0.18f;
            m_device.UpdateBuffer(
                *m_lodDebugMaterial->ParametersBuffer,
                &m_lodDebugMaterial->Parameters,
                sizeof(m_lodDebugMaterial->Parameters));

            Transform overlayTransform{};
            overlayTransform.Position = proxy.WorldBounds.Center;
            overlayTransform.Scale =
            {
                proxy.WorldBounds.Radius * 1.05f,
                proxy.WorldBounds.Radius * 1.05f,
                proxy.WorldBounds.Radius * 1.05f
            };

            const Pragma::Math::Matrix4 world = ToMatrix(overlayTransform);
            MainInstanceConstants instanceConstants{};
            instanceConstants.WorldViewProjection[0] = Pragma::Math::Multiply(world, viewProjection);
            instanceConstants.World[0] = world;
            instanceConstants.WorldNoScale[0] = world;
            instanceConstants.WorldLightClip[0] = hasDirectionalLight ? Pragma::Math::Multiply(world, lightViewProjection) : Pragma::Math::Identity();
            m_device.UpdateBuffer(*m_mainInstanceConstantBuffer, &instanceConstants, sizeof(instanceConstants));

            m_commandList->SetGraphicsPipeline(*m_lodDebugMaterial->Pipeline);
            ++m_lastFrameStatistics.PipelineBinds;
            m_commandList->SetVertexBuffer(*m_debugCubeMesh->VertexBuffer, 0);
            ++m_lastFrameStatistics.VertexBufferBinds;
            m_commandList->SetIndexBuffer(*m_debugCubeMesh->IndexBuffer, m_debugCubeMesh->IndexFormat, 0);
            ++m_lastFrameStatistics.IndexBufferBinds;
            m_commandList->SetConstantBuffer(1, *m_lodDebugMaterial->ParametersBuffer);
            ++m_lastFrameStatistics.MaterialBufferBinds;
            m_commandList->SetConstantBuffer(2, *m_mainInstanceConstantBuffer);
            m_commandList->SetTexture(0, nullptr);
            ++m_lastFrameStatistics.TextureBinds;
            m_commandList->DrawIndexed(m_debugCubeMesh->IndexCount, 0, 0);
            ++m_lastFrameStatistics.LodOverlayDrawCalls;
            ++m_lastFrameStatistics.TotalDrawCalls;
            const std::uint64_t triangles = static_cast<std::uint64_t>(m_debugCubeMesh->IndexCount / 3);
            m_lastFrameStatistics.TotalTriangles += triangles;
        }
        m_device.EndGpuScope();
    }

    m_commandList->SetTexture(0, nullptr);
    ++m_lastFrameStatistics.TextureBinds;
    m_commandList->SetTexture(1, nullptr);
    ++m_lastFrameStatistics.TextureBinds;
    m_commandList->SetTexture(2, nullptr);
    ++m_lastFrameStatistics.TextureBinds;
    m_commandList->SetTexture(3, nullptr);
    ++m_lastFrameStatistics.TextureBinds;
    m_commandList->SetTexture(4, nullptr);
    ++m_lastFrameStatistics.TextureBinds;

}

Pragma::RHI::BackendType RenderSystem::GetBackendType() const noexcept
{
    return m_device.GetBackendType();
}

const RenderStatistics& RenderSystem::GetLastFrameStatistics() const noexcept
{
    return m_lastFrameStatistics;
}

Pragma::Core::GraphicsQualityPreset RenderSystem::GetGraphicsQualityPreset() const noexcept
{
    return m_graphicsConfig.QualityPreset;
}

const Pragma::Core::GraphicsConfig& RenderSystem::GetGraphicsConfig() const noexcept
{
    return m_graphicsConfig;
}

void RenderSystem::SetGraphicsQualityPreset(const Pragma::Core::GraphicsQualityPreset preset)
{
    if (m_graphicsConfig.QualityPreset == preset)
    {
        return;
    }

    m_graphicsConfig.QualityPreset = preset;
    ApplyGraphicsQualityPreset(m_graphicsConfig);
    SanitizeGraphicsConfig(m_graphicsConfig);
    CreateRenderTargets(m_swapchainDesc.Extent);
    Pragma::Core::Log(
        Pragma::Core::LogCategory::Renderer,
        Pragma::Core::LogLevel::Info,
        "Graphics quality preset switched to " + std::string(Pragma::Core::ToString(preset)) + ".");
}

void RenderSystem::ApplyGraphicsConfig(const Pragma::Core::GraphicsConfig& graphicsConfig)
{
    m_graphicsConfig = graphicsConfig;
    if (m_graphicsConfig.QualityPreset != Pragma::Core::GraphicsQualityPreset::Custom)
    {
        ApplyGraphicsQualityPreset(m_graphicsConfig);
    }
    SanitizeGraphicsConfig(m_graphicsConfig);
    CreateRenderTargets(m_swapchainDesc.Extent);
    Pragma::Core::Log(
        Pragma::Core::LogCategory::Renderer,
        Pragma::Core::LogLevel::Info,
        "Graphics settings applied. Preset=" + std::string(Pragma::Core::ToString(m_graphicsConfig.QualityPreset)) + ".");
}

void RenderSystem::CreateRenderTargets(const Pragma::RHI::Extent2D extent)
{
    if (extent.Width == 0 || extent.Height == 0)
    {
        return;
    }

    const Pragma::RHI::Extent2D internalExtent = ResolveInternalRenderExtent(extent, m_graphicsConfig.RenderScale);

    Pragma::RHI::TextureDesc hdrColorDesc;
    hdrColorDesc.Format = Pragma::RHI::PixelFormat::R16G16B16A16_Float;
    hdrColorDesc.Width = internalExtent.Width;
    hdrColorDesc.Height = internalExtent.Height;
    hdrColorDesc.BindMask = Pragma::RHI::Bind_RenderTarget | Pragma::RHI::Bind_ShaderResource;

    Pragma::RHI::TextureDesc hdrDepthDesc;
    hdrDepthDesc.Format = m_swapchainDesc.DepthFormat;
    hdrDepthDesc.Width = internalExtent.Width;
    hdrDepthDesc.Height = internalExtent.Height;
    hdrDepthDesc.BindMask = Pragma::RHI::Bind_DepthStencil;

    Pragma::RHI::TextureDesc bloomColorDesc;
    bloomColorDesc.Format = Pragma::RHI::PixelFormat::R16G16B16A16_Float;
    const float bloomResolutionScale = std::clamp(m_graphicsConfig.BloomResolutionScale, 0.125f, 0.5f);
    bloomColorDesc.Width = std::max(1u, static_cast<std::uint32_t>(std::lround(static_cast<double>(internalExtent.Width) * bloomResolutionScale)));
    bloomColorDesc.Height = std::max(1u, static_cast<std::uint32_t>(std::lround(static_cast<double>(internalExtent.Height) * bloomResolutionScale)));
    bloomColorDesc.BindMask = Pragma::RHI::Bind_RenderTarget | Pragma::RHI::Bind_ShaderResource;

    const ShadowQualitySettings shadowSettings = ResolveShadowQualitySettings(m_graphicsConfig.ShadowQuality);

    Pragma::RHI::TextureDesc shadowColorDesc;
    shadowColorDesc.Format = Pragma::RHI::PixelFormat::R16G16B16A16_Float;
    shadowColorDesc.Width = shadowSettings.MapResolution;
    shadowColorDesc.Height = shadowSettings.MapResolution;
    shadowColorDesc.BindMask = Pragma::RHI::Bind_RenderTarget | Pragma::RHI::Bind_ShaderResource;

    Pragma::RHI::TextureDesc shadowDepthDesc;
    shadowDepthDesc.Format = Pragma::RHI::PixelFormat::D24_UNorm_S8_UInt;
    shadowDepthDesc.Width = shadowSettings.MapResolution;
    shadowDepthDesc.Height = shadowSettings.MapResolution;
    shadowDepthDesc.BindMask = Pragma::RHI::Bind_DepthStencil;

    m_hdrColorTarget = m_device.CreateTexture(hdrColorDesc, nullptr);
    m_hdrDepthTarget = m_device.CreateTexture(hdrDepthDesc, nullptr);
    m_bloomColorTarget = m_graphicsConfig.BloomEnabled ? m_device.CreateTexture(bloomColorDesc, nullptr) : nullptr;
    m_shadowColorTarget = m_device.CreateTexture(shadowColorDesc, nullptr);
    m_shadowDepthTarget = m_device.CreateTexture(shadowDepthDesc, nullptr);
}
}
