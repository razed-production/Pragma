#include "Pragma/Assets/AssetManager.h"

#include "Pragma/Assets/ImageLoader.h"
#include "Pragma/Assets/MaterialLoader.h"
#include "Pragma/Assets/ObjLoader.h"
#include "Pragma/Core/Log.h"
#include "Pragma/RHI/Device.h"
#include "Pragma/Renderer/Primitives.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace Pragma::Assets
{
namespace
{
std::uint32_t ComputeMipLevelCount(const std::uint32_t width, const std::uint32_t height) noexcept
{
    std::uint32_t mipLevels = 1;
    std::uint32_t currentWidth = width;
    std::uint32_t currentHeight = height;

    while (currentWidth > 1 || currentHeight > 1)
    {
        currentWidth = std::max(1u, currentWidth / 2u);
        currentHeight = std::max(1u, currentHeight / 2u);
        ++mipLevels;
    }

    return mipLevels;
}

std::vector<std::uint8_t> GenerateNextMipLevel(
    const std::vector<std::uint8_t>& sourcePixels,
    const std::uint32_t sourceWidth,
    const std::uint32_t sourceHeight,
    const bool srgbColorData)
{
    const std::uint32_t targetWidth = std::max(1u, sourceWidth / 2u);
    const std::uint32_t targetHeight = std::max(1u, sourceHeight / 2u);
    std::vector<std::uint8_t> targetPixels(static_cast<std::size_t>(targetWidth) * static_cast<std::size_t>(targetHeight) * 4u, 0u);

    auto loadPixel = [&](const std::uint32_t x, const std::uint32_t y) noexcept -> std::array<float, 4>
    {
        const std::uint32_t clampedX = std::min(x, sourceWidth - 1u);
        const std::uint32_t clampedY = std::min(y, sourceHeight - 1u);
        const std::size_t baseIndex = (static_cast<std::size_t>(clampedY) * static_cast<std::size_t>(sourceWidth) + static_cast<std::size_t>(clampedX)) * 4u;

        auto toLinear = [&](const float value) noexcept
        {
            return srgbColorData ? std::pow(value, 2.2f) : value;
        };

        return
        {
            toLinear(static_cast<float>(sourcePixels[baseIndex + 0u]) / 255.0f),
            toLinear(static_cast<float>(sourcePixels[baseIndex + 1u]) / 255.0f),
            toLinear(static_cast<float>(sourcePixels[baseIndex + 2u]) / 255.0f),
            static_cast<float>(sourcePixels[baseIndex + 3u]) / 255.0f
        };
    };

    auto toByte = [&](const float linear) noexcept -> std::uint8_t
    {
        const float clamped = std::clamp(linear, 0.0f, 1.0f);
        const float encoded = srgbColorData ? std::pow(clamped, 1.0f / 2.2f) : clamped;
        return static_cast<std::uint8_t>(std::round(encoded * 255.0f));
    };

    auto toLinearByte = [](const float linear) noexcept -> std::uint8_t
    {
        return static_cast<std::uint8_t>(std::round(std::clamp(linear, 0.0f, 1.0f) * 255.0f));
    };

    for (std::uint32_t y = 0; y < targetHeight; ++y)
    {
        for (std::uint32_t x = 0; x < targetWidth; ++x)
        {
            const auto sample0 = loadPixel(x * 2u + 0u, y * 2u + 0u);
            const auto sample1 = loadPixel(x * 2u + 1u, y * 2u + 0u);
            const auto sample2 = loadPixel(x * 2u + 0u, y * 2u + 1u);
            const auto sample3 = loadPixel(x * 2u + 1u, y * 2u + 1u);

            std::array<float, 4> averaged{};
            for (std::size_t component = 0; component < averaged.size(); ++component)
            {
                averaged[component] = (sample0[component] + sample1[component] + sample2[component] + sample3[component]) * 0.25f;
            }

            const std::size_t targetIndex =
                (static_cast<std::size_t>(y) * static_cast<std::size_t>(targetWidth) + static_cast<std::size_t>(x)) * 4u;
            targetPixels[targetIndex + 0u] = toByte(averaged[0]);
            targetPixels[targetIndex + 1u] = toByte(averaged[1]);
            targetPixels[targetIndex + 2u] = toByte(averaged[2]);
            targetPixels[targetIndex + 3u] = toLinearByte(averaged[3]);
        }
    }

    return targetPixels;
}

const char* ToTextureCacheSuffix(const TextureColorSpace colorSpace) noexcept
{
    switch (colorSpace)
    {
    case TextureColorSpace::LinearData:
        return "|linear";
    case TextureColorSpace::Color:
    default:
        return "|color";
    }
}
}

AssetManager::AssetManager(Pragma::RHI::IDevice& device, AssetManifest manifest)
    : m_device(device)
    , m_manifest(std::move(manifest))
{
}

std::filesystem::path AssetManager::ResolvePath(const AssetId& assetId) const
{
    return m_manifest.ResolvePath(assetId);
}

std::vector<AssetId> AssetManager::GetAssetIdsByPrefix(const std::string& prefix) const
{
    return m_manifest.FindAssetIdsByPrefix(prefix);
}

MaterialAssetData AssetManager::GetMaterialAssetData(const AssetId& assetId) const
{
    return LoadMaterialAsset(m_manifest.ResolvePath(assetId));
}

bool AssetManager::SaveMaterialAssetData(const AssetId& assetId, const MaterialAssetData& assetData)
{
    const std::filesystem::path assetPath = m_manifest.ResolvePath(assetId);
    SaveMaterialAsset(assetData, assetPath);

    const std::string cacheKey = assetId.Value;
    if (const auto it = m_materialCache.find(cacheKey); it != m_materialCache.end())
    {
        if (std::shared_ptr<Pragma::Renderer::Material> cachedMaterial = it->second.lock())
        {
            for (std::size_t i = 0; i < 4; ++i)
            {
                cachedMaterial->Parameters.BaseColor[i] = assetData.BaseColor[i];
            }
            for (std::size_t i = 0; i < 3; ++i)
            {
                cachedMaterial->Parameters.EmissiveColor[i] = assetData.EmissiveColor[i];
            }
            cachedMaterial->Parameters.Roughness = assetData.Roughness;
            cachedMaterial->Parameters.Metallic = assetData.Metallic;
            cachedMaterial->Parameters.AmbientOcclusion = assetData.AmbientOcclusion;
            cachedMaterial->Parameters.UseAlbedoTexture = assetData.UseAlbedoTexture ? 1.0f : 0.0f;
            cachedMaterial->Parameters.EmissiveIntensity = assetData.EmissiveIntensity;
            cachedMaterial->Parameters.UseNormalTexture = assetData.UseNormalTexture ? 1.0f : 0.0f;
            cachedMaterial->Parameters.UseOrmTexture = assetData.UseOrmTexture ? 1.0f : 0.0f;
            cachedMaterial->Parameters.UseEmissiveTexture = assetData.UseEmissiveTexture ? 1.0f : 0.0f;
            cachedMaterial->Parameters.NormalStrength = assetData.NormalStrength;
            cachedMaterial->AlbedoTexture.reset();
            cachedMaterial->NormalTexture.reset();
            cachedMaterial->OrmTexture.reset();
            cachedMaterial->EmissiveTexture.reset();

            if (assetData.UseAlbedoTexture && !assetData.AlbedoTextureAsset.empty())
            {
                cachedMaterial->AlbedoTexture = LoadTexture(assetData.AlbedoTextureAsset, TextureColorSpace::Color);
            }
            if (assetData.UseNormalTexture && !assetData.NormalTextureAsset.empty())
            {
                cachedMaterial->NormalTexture = LoadTexture(assetData.NormalTextureAsset, TextureColorSpace::LinearData);
            }
            if (assetData.UseOrmTexture && !assetData.OrmTextureAsset.empty())
            {
                cachedMaterial->OrmTexture = LoadTexture(assetData.OrmTextureAsset, TextureColorSpace::LinearData);
            }
            if (assetData.UseEmissiveTexture && !assetData.EmissiveTextureAsset.empty())
            {
                cachedMaterial->EmissiveTexture = LoadTexture(assetData.EmissiveTextureAsset, TextureColorSpace::Color);
            }

            m_device.UpdateBuffer(*cachedMaterial->ParametersBuffer, &cachedMaterial->Parameters, sizeof(cachedMaterial->Parameters));
        }
    }

    return true;
}

bool AssetManager::RegisterAsset(const AssetId& assetId, const std::filesystem::path& relativePath)
{
    m_manifest.RegisterAsset(assetId, relativePath);
    m_manifest.Save();
    return true;
}

std::shared_ptr<Pragma::Renderer::Mesh> AssetManager::LoadMesh(const AssetId& assetId)
{
    const std::string cacheKey = assetId.Value;

    if (const auto it = m_meshCache.find(cacheKey); it != m_meshCache.end())
    {
        if (std::shared_ptr<Pragma::Renderer::Mesh> cachedMesh = it->second.lock())
        {
            return cachedMesh;
        }
    }

    const std::filesystem::path assetPath = m_manifest.ResolvePath(assetId);
    Pragma::Core::Log(Pragma::Core::LogLevel::Info, "Loading mesh asset: " + cacheKey);
    const MeshAssetData assetData = LoadObjMesh(assetPath);
    std::shared_ptr<Pragma::Renderer::Mesh> mesh = UploadMesh(assetData);
    m_meshCache[cacheKey] = mesh;
    return mesh;
}

std::shared_ptr<Pragma::Renderer::Material> AssetManager::LoadMaterial(const AssetId& assetId)
{
    const std::string cacheKey = assetId.Value;

    if (const auto it = m_materialCache.find(cacheKey); it != m_materialCache.end())
    {
        if (std::shared_ptr<Pragma::Renderer::Material> cachedMaterial = it->second.lock())
        {
            return cachedMaterial;
        }
    }

    const std::filesystem::path assetPath = m_manifest.ResolvePath(assetId);
    Pragma::Core::Log(Pragma::Core::LogLevel::Info, "Loading material asset: " + cacheKey);
    const MaterialAssetData assetData = LoadMaterialAsset(assetPath);
    std::shared_ptr<Pragma::Renderer::Material> material = UploadMaterial(assetData);
    m_materialCache[cacheKey] = material;
    return material;
}

std::shared_ptr<Pragma::RHI::ITexture> AssetManager::LoadTexture(const AssetId& assetId, const TextureColorSpace colorSpace)
{
    const std::string cacheKey = assetId.Value + ToTextureCacheSuffix(colorSpace);

    if (const auto it = m_textureCache.find(cacheKey); it != m_textureCache.end())
    {
        if (std::shared_ptr<Pragma::RHI::ITexture> cachedTexture = it->second.lock())
        {
            return cachedTexture;
        }
    }

    const std::filesystem::path assetPath = m_manifest.ResolvePath(assetId);
    Pragma::Core::Log(Pragma::Core::LogLevel::Info, "Loading texture asset: " + cacheKey);
    const Pragma::Core::Result<ImageAssetData> imageResult = LoadPpmImage(assetPath);
    if (!imageResult)
    {
        Pragma::Core::Log(Pragma::Core::LogLevel::Error, imageResult.GetError());
        throw std::runtime_error(imageResult.GetError());
    }

    std::shared_ptr<Pragma::RHI::ITexture> texture = UploadTexture(imageResult.GetValue(), colorSpace);
    m_textureCache[cacheKey] = texture;
    return texture;
}

std::shared_ptr<Pragma::Renderer::Mesh> AssetManager::UploadMesh(const MeshAssetData& assetData)
{
    if (assetData.Vertices.empty() || assetData.Indices.empty())
    {
        throw std::runtime_error("Cannot upload an empty mesh asset.");
    }

    Pragma::RHI::BufferDesc vertexBufferDesc;
    vertexBufferDesc.SizeInBytes = static_cast<std::uint64_t>(assetData.Vertices.size() * sizeof(assetData.Vertices[0]));
    vertexBufferDesc.Stride = sizeof(assetData.Vertices[0]);
    vertexBufferDesc.BindMask = Pragma::RHI::Bind_VertexBuffer;

    Pragma::RHI::BufferDesc indexBufferDesc;
    indexBufferDesc.SizeInBytes = static_cast<std::uint64_t>(assetData.Indices.size() * sizeof(assetData.Indices[0]));
    indexBufferDesc.Stride = sizeof(assetData.Indices[0]);
    indexBufferDesc.BindMask = Pragma::RHI::Bind_IndexBuffer;

    auto mesh = std::make_shared<Pragma::Renderer::Mesh>();
    mesh->VertexBuffer = m_device.CreateBuffer(vertexBufferDesc, assetData.Vertices.data());
    mesh->IndexBuffer = m_device.CreateBuffer(indexBufferDesc, assetData.Indices.data());
    mesh->IndexFormat = Pragma::RHI::IndexFormat::UInt32;
    mesh->IndexCount = static_cast<std::uint32_t>(assetData.Indices.size());
    mesh->LocalBoundsCenter = assetData.LocalBoundsCenter;
    mesh->LocalBoundsRadius = assetData.LocalBoundsRadius;
    return mesh;
}

std::shared_ptr<Pragma::Renderer::Material> AssetManager::UploadMaterial(const MaterialAssetData& assetData)
{
    std::shared_ptr<Pragma::Renderer::Material> material = Pragma::Renderer::CreateDefaultMaterial(m_device);
    for (std::size_t i = 0; i < 4; ++i)
    {
        material->Parameters.BaseColor[i] = assetData.BaseColor[i];
    }
    for (std::size_t i = 0; i < 3; ++i)
    {
        material->Parameters.EmissiveColor[i] = assetData.EmissiveColor[i];
    }
    material->Parameters.Roughness = assetData.Roughness;
    material->Parameters.Metallic = assetData.Metallic;
    material->Parameters.AmbientOcclusion = assetData.AmbientOcclusion;
    material->Parameters.UseAlbedoTexture = assetData.UseAlbedoTexture ? 1.0f : 0.0f;
    material->Parameters.EmissiveIntensity = assetData.EmissiveIntensity;
    material->Parameters.UseNormalTexture = assetData.UseNormalTexture ? 1.0f : 0.0f;
    material->Parameters.UseOrmTexture = assetData.UseOrmTexture ? 1.0f : 0.0f;
    material->Parameters.UseEmissiveTexture = assetData.UseEmissiveTexture ? 1.0f : 0.0f;
    material->Parameters.NormalStrength = assetData.NormalStrength;

    if (assetData.UseAlbedoTexture && !assetData.AlbedoTextureAsset.empty())
    {
        material->AlbedoTexture = LoadTexture(assetData.AlbedoTextureAsset, TextureColorSpace::Color);
    }
    if (assetData.UseNormalTexture && !assetData.NormalTextureAsset.empty())
    {
        material->NormalTexture = LoadTexture(assetData.NormalTextureAsset, TextureColorSpace::LinearData);
    }
    if (assetData.UseOrmTexture && !assetData.OrmTextureAsset.empty())
    {
        material->OrmTexture = LoadTexture(assetData.OrmTextureAsset, TextureColorSpace::LinearData);
    }
    if (assetData.UseEmissiveTexture && !assetData.EmissiveTextureAsset.empty())
    {
        material->EmissiveTexture = LoadTexture(assetData.EmissiveTextureAsset, TextureColorSpace::Color);
    }

    m_device.UpdateBuffer(*material->ParametersBuffer, &material->Parameters, sizeof(material->Parameters));
    return material;
}

std::shared_ptr<Pragma::RHI::ITexture> AssetManager::UploadTexture(
    const ImageAssetData& assetData,
    const TextureColorSpace colorSpace)
{
    if (assetData.Width == 0 || assetData.Height == 0 || assetData.Pixels.empty())
    {
        throw std::runtime_error("Cannot upload an empty texture asset.");
    }

    Pragma::RHI::TextureDesc textureDesc;
    textureDesc.Dimension = Pragma::RHI::TextureDimension::Texture2D;
    textureDesc.Format = Pragma::RHI::PixelFormat::R8G8B8A8_UNorm;
    textureDesc.Width = assetData.Width;
    textureDesc.Height = assetData.Height;
    textureDesc.DepthOrArraySize = 1;
    textureDesc.MipLevels = ComputeMipLevelCount(assetData.Width, assetData.Height);
    textureDesc.SampleCount = 1;
    textureDesc.BindMask = Pragma::RHI::Bind_ShaderResource;

    std::vector<std::vector<std::uint8_t>> mipChain;
    mipChain.reserve(textureDesc.MipLevels);
    mipChain.push_back(assetData.Pixels);

    std::uint32_t mipWidth = assetData.Width;
    std::uint32_t mipHeight = assetData.Height;
    while (mipChain.size() < textureDesc.MipLevels)
    {
        mipChain.push_back(GenerateNextMipLevel(
            mipChain.back(),
            mipWidth,
            mipHeight,
            colorSpace == TextureColorSpace::Color));
        mipWidth = std::max(1u, mipWidth / 2u);
        mipHeight = std::max(1u, mipHeight / 2u);
    }

    std::vector<Pragma::RHI::TextureSubresourceData> subresources;
    subresources.reserve(textureDesc.MipLevels);
    mipWidth = assetData.Width;
    mipHeight = assetData.Height;
    for (const std::vector<std::uint8_t>& mipPixels : mipChain)
    {
        Pragma::RHI::TextureSubresourceData subresource{};
        subresource.Data = mipPixels.data();
        subresource.RowPitch = mipWidth * 4u;
        subresource.SlicePitch = mipWidth * mipHeight * 4u;
        subresources.push_back(subresource);

        mipWidth = std::max(1u, mipWidth / 2u);
        mipHeight = std::max(1u, mipHeight / 2u);
    }

    return std::shared_ptr<Pragma::RHI::ITexture>(m_device.CreateTexture(textureDesc, subresources.data()).release());
}
}
