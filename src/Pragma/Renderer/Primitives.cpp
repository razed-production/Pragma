#include "Pragma/Renderer/Primitives.h"

#include "Pragma/RHI/Device.h"
#include "Pragma/Math/Vector3.h"
#include "Pragma/Renderer/ShaderSource.h"
#include "Pragma/Renderer/Vertex.h"

#include <cmath>
#include <iterator>

namespace Pragma::Renderer
{
std::shared_ptr<Mesh> CreateCubeMesh(Pragma::RHI::IDevice& device)
{
    static constexpr VertexPCN kCubeVertices[] =
    {
        { { -1.0f, -1.0f, -1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f } },
        { { -1.0f,  1.0f, -1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f } },
        { {  1.0f,  1.0f, -1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f } },
        { {  1.0f, -1.0f, -1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 1.0f } },
        { { -1.0f, -1.0f,  1.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
        { { -1.0f,  1.0f,  1.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
        { {  1.0f,  1.0f,  1.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
        { {  1.0f, -1.0f,  1.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } }
    };

    static constexpr std::uint32_t kCubeIndices[] =
    {
        0, 1, 2, 0, 2, 3,
        4, 6, 5, 4, 7, 6,
        0, 4, 5, 0, 5, 1,
        3, 2, 6, 3, 6, 7,
        1, 5, 6, 1, 6, 2,
        0, 3, 7, 0, 7, 4
    };

    Pragma::RHI::BufferDesc vertexBufferDesc;
    vertexBufferDesc.SizeInBytes = sizeof(kCubeVertices);
    vertexBufferDesc.Stride = sizeof(VertexPCN);
    vertexBufferDesc.BindMask = Pragma::RHI::Bind_VertexBuffer;

    Pragma::RHI::BufferDesc indexBufferDesc;
    indexBufferDesc.SizeInBytes = sizeof(kCubeIndices);
    indexBufferDesc.Stride = sizeof(std::uint32_t);
    indexBufferDesc.BindMask = Pragma::RHI::Bind_IndexBuffer;

    auto mesh = std::make_shared<Mesh>();
    mesh->VertexBuffer = device.CreateBuffer(vertexBufferDesc, kCubeVertices);
    mesh->IndexBuffer = device.CreateBuffer(indexBufferDesc, kCubeIndices);
    mesh->IndexFormat = Pragma::RHI::IndexFormat::UInt32;
    mesh->IndexCount = static_cast<std::uint32_t>(std::size(kCubeIndices));
    mesh->LocalBoundsCenter = { 0.0f, 0.0f, 0.0f };
    mesh->LocalBoundsRadius = std::sqrt(3.0f);
    return mesh;
}

std::shared_ptr<Material> CreateDefaultMaterial(Pragma::RHI::IDevice& device)
{
    static const std::string s_vertexShaderSource = LoadRendererShaderSource("lit_default_vs.hlsl");
    static const std::string s_pixelShaderSource = LoadRendererShaderSource("lit_default_ps.hlsl");

    static constexpr Pragma::RHI::VertexAttributeDesc kVertexAttributes[] =
    {
        { "POSITION", 0, Pragma::RHI::PixelFormat::R32G32B32_Float, 0 },
        { "COLOR", 0, Pragma::RHI::PixelFormat::R32G32B32_Float, 12 },
        { "NORMAL", 0, Pragma::RHI::PixelFormat::R32G32B32_Float, 24 },
        { "TEXCOORD", 0, Pragma::RHI::PixelFormat::R32G32_Float, 36 }
    };

    Pragma::RHI::GraphicsPipelineDesc pipelineDesc;
    pipelineDesc.DebugName = "LitDefault3D";
    pipelineDesc.VertexAttributes = kVertexAttributes;
    pipelineDesc.VertexAttributeCount = static_cast<std::uint32_t>(std::size(kVertexAttributes));
    pipelineDesc.VertexStride = sizeof(VertexPCN);
    pipelineDesc.ColorFormat = Pragma::RHI::PixelFormat::R16G16B16A16_Float;
    pipelineDesc.Rasterizer.FrontCounterClockwise = true;
    pipelineDesc.VertexShader.Stage = Pragma::RHI::ShaderStage::Vertex;
    pipelineDesc.VertexShader.SourceCode = s_vertexShaderSource.c_str();
    pipelineDesc.VertexShader.DebugName = "LitDefaultVS";
    pipelineDesc.PixelShader.Stage = Pragma::RHI::ShaderStage::Pixel;
    pipelineDesc.PixelShader.SourceCode = s_pixelShaderSource.c_str();
    pipelineDesc.PixelShader.DebugName = "LitDefaultPS";

    Pragma::RHI::BufferDesc materialBufferDesc;
    materialBufferDesc.SizeInBytes = sizeof(MaterialParameters);
    materialBufferDesc.Stride = sizeof(MaterialParameters);
    materialBufferDesc.BindMask = Pragma::RHI::Bind_ConstantBuffer;
    materialBufferDesc.Usage = Pragma::RHI::ResourceUsage::Dynamic;
    materialBufferDesc.CpuWritable = true;

    auto material = std::make_shared<Material>();
    material->Pipeline = device.CreateGraphicsPipeline(pipelineDesc);
    material->Parameters.BaseColor[0] = 1.0f;
    material->Parameters.BaseColor[1] = 1.0f;
    material->Parameters.BaseColor[2] = 1.0f;
    material->Parameters.BaseColor[3] = 1.0f;
    material->Parameters.EmissiveColor[0] = 0.0f;
    material->Parameters.EmissiveColor[1] = 0.0f;
    material->Parameters.EmissiveColor[2] = 0.0f;
    material->Parameters.Roughness = 0.55f;
    material->Parameters.Metallic = 0.0f;
    material->Parameters.AmbientOcclusion = 1.0f;
    material->Parameters.UseAlbedoTexture = 0.0f;
    material->Parameters.EmissiveIntensity = 0.0f;
    material->Parameters.UseNormalTexture = 0.0f;
    material->Parameters.UseOrmTexture = 0.0f;
    material->Parameters.UseEmissiveTexture = 0.0f;
    material->Parameters.NormalStrength = 1.0f;
    material->ParametersBuffer = device.CreateBuffer(materialBufferDesc, nullptr);
    device.UpdateBuffer(*material->ParametersBuffer, &material->Parameters, sizeof(material->Parameters));
    return material;
}

std::shared_ptr<Material> CreateWireframeDebugMaterial(Pragma::RHI::IDevice& device)
{
    static const std::string s_vertexShaderSource = LoadRendererShaderSource("lit_default_vs.hlsl");
    static const std::string s_pixelShaderSource = LoadRendererShaderSource("lit_default_ps.hlsl");

    static constexpr Pragma::RHI::VertexAttributeDesc kVertexAttributes[] =
    {
        { "POSITION", 0, Pragma::RHI::PixelFormat::R32G32B32_Float, 0 },
        { "COLOR", 0, Pragma::RHI::PixelFormat::R32G32B32_Float, 12 },
        { "NORMAL", 0, Pragma::RHI::PixelFormat::R32G32B32_Float, 24 },
        { "TEXCOORD", 0, Pragma::RHI::PixelFormat::R32G32_Float, 36 }
    };

    Pragma::RHI::GraphicsPipelineDesc pipelineDesc;
    pipelineDesc.DebugName = "WireframeDebug3D";
    pipelineDesc.VertexAttributes = kVertexAttributes;
    pipelineDesc.VertexAttributeCount = static_cast<std::uint32_t>(std::size(kVertexAttributes));
    pipelineDesc.VertexStride = sizeof(VertexPCN);
    pipelineDesc.ColorFormat = Pragma::RHI::PixelFormat::R16G16B16A16_Float;
    pipelineDesc.Rasterizer.Fill = Pragma::RHI::FillMode::Wireframe;
    pipelineDesc.Rasterizer.Cull = Pragma::RHI::CullMode::None;
    pipelineDesc.Rasterizer.FrontCounterClockwise = true;
    pipelineDesc.VertexShader.Stage = Pragma::RHI::ShaderStage::Vertex;
    pipelineDesc.VertexShader.SourceCode = s_vertexShaderSource.c_str();
    pipelineDesc.VertexShader.DebugName = "WireframeDebugVS";
    pipelineDesc.PixelShader.Stage = Pragma::RHI::ShaderStage::Pixel;
    pipelineDesc.PixelShader.SourceCode = s_pixelShaderSource.c_str();
    pipelineDesc.PixelShader.DebugName = "WireframeDebugPS";

    Pragma::RHI::BufferDesc materialBufferDesc;
    materialBufferDesc.SizeInBytes = sizeof(MaterialParameters);
    materialBufferDesc.Stride = sizeof(MaterialParameters);
    materialBufferDesc.BindMask = Pragma::RHI::Bind_ConstantBuffer;
    materialBufferDesc.Usage = Pragma::RHI::ResourceUsage::Dynamic;
    materialBufferDesc.CpuWritable = true;

    auto material = std::make_shared<Material>();
    material->Pipeline = device.CreateGraphicsPipeline(pipelineDesc);
    material->Parameters.BaseColor[0] = 1.0f;
    material->Parameters.BaseColor[1] = 1.0f;
    material->Parameters.BaseColor[2] = 1.0f;
    material->Parameters.BaseColor[3] = 1.0f;
    material->Parameters.EmissiveColor[0] = 0.0f;
    material->Parameters.EmissiveColor[1] = 0.0f;
    material->Parameters.EmissiveColor[2] = 0.0f;
    material->Parameters.Roughness = 0.25f;
    material->Parameters.Metallic = 0.0f;
    material->Parameters.AmbientOcclusion = 1.0f;
    material->Parameters.UseAlbedoTexture = 0.0f;
    material->Parameters.EmissiveIntensity = 0.0f;
    material->Parameters.UseNormalTexture = 0.0f;
    material->Parameters.UseOrmTexture = 0.0f;
    material->Parameters.UseEmissiveTexture = 0.0f;
    material->Parameters.NormalStrength = 1.0f;
    material->ParametersBuffer = device.CreateBuffer(materialBufferDesc, nullptr);
    device.UpdateBuffer(*material->ParametersBuffer, &material->Parameters, sizeof(material->Parameters));
    return material;
}
}
