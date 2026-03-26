#include "Pragma/Renderer/Primitives.h"

#include "Pragma/RHI/Device.h"
#include "Pragma/Renderer/Vertex.h"

#include <iterator>

namespace Pragma::Renderer
{
namespace
{
constexpr char kVertexShaderSource[] = R"(
cbuffer FrameConstants : register(b0)
{
    row_major float4x4 WorldViewProjection;
    row_major float4x4 World;
    float3 LightDirection;
    float LightIntensity;
    float3 LightColor;
    float Padding0;
};

cbuffer MaterialConstants : register(b1)
{
    float4 BaseColor;
    float Roughness;
    float UseAlbedoTexture;
    float2 Padding1;
};

Texture2D AlbedoTexture : register(t0);
SamplerState LinearSampler : register(s0);

struct VSInput
{
    float3 position : POSITION;
    float3 color : COLOR;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 color : COLOR;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD;
};

PSInput main(VSInput input)
{
    PSInput output;
    output.position = mul(float4(input.position, 1.0f), WorldViewProjection);
    output.color = input.color * BaseColor.rgb;
    output.normal = mul(float4(input.normal, 0.0f), World).xyz;
    output.texcoord = input.texcoord;
    return output;
}
)";

constexpr char kPixelShaderSource[] = R"(
cbuffer FrameConstants : register(b0)
{
    row_major float4x4 WorldViewProjection;
    row_major float4x4 World;
    float3 LightDirection;
    float LightIntensity;
    float3 LightColor;
    float Padding0;
};

cbuffer MaterialConstants : register(b1)
{
    float4 BaseColor;
    float Roughness;
    float UseAlbedoTexture;
    float2 Padding1;
};

Texture2D AlbedoTexture : register(t0);
SamplerState LinearSampler : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float3 color : COLOR;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD;
};

float4 main(PSInput input) : SV_TARGET
{
    float3 normal = normalize(input.normal);
    float3 lightDirection = normalize(LightDirection);
    float3 albedo = input.color * BaseColor.rgb;
    if (UseAlbedoTexture > 0.5f)
    {
        albedo *= AlbedoTexture.Sample(LinearSampler, input.texcoord).rgb;
    }
    float diffuse = saturate(dot(normal, -lightDirection)) * LightIntensity;
    float ambient = 0.15f + (1.0f - Roughness) * 0.1f;
    float3 litColor = albedo * (ambient + diffuse * LightColor);
    return float4(litColor, BaseColor.a);
}
)";
}

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
    return mesh;
}

std::shared_ptr<Material> CreateDefaultMaterial(Pragma::RHI::IDevice& device)
{
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
    pipelineDesc.VertexShader.Stage = Pragma::RHI::ShaderStage::Vertex;
    pipelineDesc.VertexShader.SourceCode = kVertexShaderSource;
    pipelineDesc.VertexShader.DebugName = "LitDefaultVS";
    pipelineDesc.PixelShader.Stage = Pragma::RHI::ShaderStage::Pixel;
    pipelineDesc.PixelShader.SourceCode = kPixelShaderSource;
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
    material->Parameters.Roughness = 0.45f;
    material->Parameters.UseAlbedoTexture = 0.0f;
    material->ParametersBuffer = device.CreateBuffer(materialBufferDesc, nullptr);
    device.UpdateBuffer(*material->ParametersBuffer, &material->Parameters, sizeof(material->Parameters));
    return material;
}
}
