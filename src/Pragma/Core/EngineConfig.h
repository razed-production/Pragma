#pragma once

#include "Pragma/RHI/BackendType.h"

namespace Pragma::Core
{
enum class GraphicsQualityPreset
{
    Performance,
    Balanced,
    Quality,
    Custom
};

enum class ShadowQualityTier
{
    Low,
    Medium,
    High,
    Ultra
};

enum class ShadingQualityTier
{
    Performance,
    Balanced,
    Quality
};

struct GraphicsConfig
{
    Pragma::RHI::BackendType Backend = Pragma::RHI::BackendType::Direct3D11;
    GraphicsQualityPreset QualityPreset = GraphicsQualityPreset::Balanced;
    ShadowQualityTier ShadowQuality = ShadowQualityTier::High;
    ShadingQualityTier ShadingQuality = ShadingQualityTier::Balanced;
    float RenderScale = 0.85f;
    float Exposure = 1.0f;
    float AmbientStrength = 0.28f;
    float EnvironmentDiffuseStrength = 0.92f;
    float EnvironmentSpecularStrength = 1.08f;
    bool BloomEnabled = true;
    float BloomResolutionScale = 0.25f;
    float BloomQuality = 1.0f;
    float BloomThreshold = 1.1f;
    float BloomIntensity = 0.08f;
    bool FxaaEnabled = true;
    float FxaaSubpixel = 0.75f;
    float FxaaEdgeThreshold = 0.166f;
    float FxaaEdgeThresholdMin = 0.0625f;
    float ShadowFilterQuality = 0.0f;
    float FogStartDistance = 18.0f;
    float FogDensity = 0.03f;
    float FogHeightFalloff = 0.09f;
    float FogMaxOpacity = 0.72f;
    bool LodEnabled = true;
    float LodNearNormalizedDistance = 14.0f;
    float LodFarNormalizedDistance = 30.0f;
    float ShadowLowLodDistanceScale = 0.65f;
};

struct EngineConfig
{
    GraphicsConfig Graphics;
};

[[nodiscard]] inline const char* ToString(const GraphicsQualityPreset preset) noexcept
{
    switch (preset)
    {
    case GraphicsQualityPreset::Performance:
        return "Performance";
    case GraphicsQualityPreset::Quality:
        return "Quality";
    case GraphicsQualityPreset::Custom:
        return "Custom";
    case GraphicsQualityPreset::Balanced:
    default:
        return "Balanced";
    }
}

[[nodiscard]] inline const char* ToString(const ShadowQualityTier tier) noexcept
{
    switch (tier)
    {
    case ShadowQualityTier::Low:
        return "Low";
    case ShadowQualityTier::Medium:
        return "Medium";
    case ShadowQualityTier::Ultra:
        return "Ultra";
    case ShadowQualityTier::High:
    default:
        return "High";
    }
}

[[nodiscard]] inline const char* ToString(const ShadingQualityTier tier) noexcept
{
    switch (tier)
    {
    case ShadingQualityTier::Performance:
        return "Performance";
    case ShadingQualityTier::Quality:
        return "Quality";
    case ShadingQualityTier::Balanced:
    default:
        return "Balanced";
    }
}
}
