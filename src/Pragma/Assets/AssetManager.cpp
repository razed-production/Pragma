#include "Pragma/Assets/AssetManager.h"

#include "Pragma/Assets/ImageLoader.h"
#include "Pragma/Assets/MaterialLoader.h"
#include "Pragma/Assets/ObjLoader.h"
#include "Pragma/Core/Log.h"
#include "Pragma/RHI/Device.h"
#include "Pragma/Renderer/Primitives.h"

#include <stdexcept>
#include <utility>

namespace Pragma::Assets
{
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
            cachedMaterial->Parameters.Roughness = assetData.Roughness;
            cachedMaterial->Parameters.UseAlbedoTexture = assetData.UseAlbedoTexture ? 1.0f : 0.0f;
            cachedMaterial->AlbedoTexture.reset();

            if (assetData.UseAlbedoTexture && !assetData.AlbedoTextureAsset.empty())
            {
                cachedMaterial->AlbedoTexture = LoadTexture(assetData.AlbedoTextureAsset);
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

std::shared_ptr<Pragma::RHI::ITexture> AssetManager::LoadTexture(const AssetId& assetId)
{
    const std::string cacheKey = assetId.Value;

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

    std::shared_ptr<Pragma::RHI::ITexture> texture = UploadTexture(imageResult.GetValue());
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
    return mesh;
}

std::shared_ptr<Pragma::Renderer::Material> AssetManager::UploadMaterial(const MaterialAssetData& assetData)
{
    std::shared_ptr<Pragma::Renderer::Material> material = Pragma::Renderer::CreateDefaultMaterial(m_device);
    for (std::size_t i = 0; i < 4; ++i)
    {
        material->Parameters.BaseColor[i] = assetData.BaseColor[i];
    }
    material->Parameters.Roughness = assetData.Roughness;
    material->Parameters.UseAlbedoTexture = assetData.UseAlbedoTexture ? 1.0f : 0.0f;

    if (assetData.UseAlbedoTexture && !assetData.AlbedoTextureAsset.empty())
    {
        material->AlbedoTexture = LoadTexture(assetData.AlbedoTextureAsset);
    }

    m_device.UpdateBuffer(*material->ParametersBuffer, &material->Parameters, sizeof(material->Parameters));
    return material;
}

std::shared_ptr<Pragma::RHI::ITexture> AssetManager::UploadTexture(const ImageAssetData& assetData)
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
    textureDesc.MipLevels = 1;
    textureDesc.SampleCount = 1;
    textureDesc.BindMask = Pragma::RHI::Bind_ShaderResource;

    Pragma::RHI::TextureSubresourceData initialData;
    initialData.Data = assetData.Pixels.data();
    initialData.RowPitch = assetData.Width * 4u;
    initialData.SlicePitch = assetData.Width * assetData.Height * 4u;

    return std::shared_ptr<Pragma::RHI::ITexture>(m_device.CreateTexture(textureDesc, &initialData).release());
}
}
