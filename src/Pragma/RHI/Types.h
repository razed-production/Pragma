#pragma once

#include <cstdint>

namespace Pragma::RHI
{
enum class PixelFormat
{
    Unknown,
    R8G8B8A8_UNorm,
    R32G32_Float,
    R32G32B32_Float,
    R16G16B16A16_Float,
    D24_UNorm_S8_UInt,
    D32_Float
};

enum class TextureDimension
{
    Texture2D,
    Texture3D,
    TextureCube
};

enum class ResourceUsage : std::uint8_t
{
    Immutable,
    Default,
    Dynamic,
    Staging
};

enum class PrimitiveTopology
{
    TriangleList,
    TriangleStrip,
    LineList
};

enum class IndexFormat
{
    UInt16,
    UInt32
};

enum class ShaderStage
{
    Vertex,
    Pixel
};

enum class CompareOp
{
    Never,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always
};

enum class FillMode
{
    Solid,
    Wireframe
};

enum class CullMode
{
    None,
    Front,
    Back
};

enum BindFlags : std::uint32_t
{
    Bind_None = 0,
    Bind_VertexBuffer = 1u << 0u,
    Bind_IndexBuffer = 1u << 1u,
    Bind_ConstantBuffer = 1u << 2u,
    Bind_ShaderResource = 1u << 3u,
    Bind_RenderTarget = 1u << 4u,
    Bind_DepthStencil = 1u << 5u,
    Bind_UnorderedAccess = 1u << 6u
};

struct BufferDesc
{
    std::uint64_t SizeInBytes = 0;
    std::uint32_t Stride = 0;
    std::uint32_t BindMask = Bind_None;
    ResourceUsage Usage = ResourceUsage::Default;
    bool CpuWritable = false;
};

struct TextureDesc
{
    TextureDimension Dimension = TextureDimension::Texture2D;
    PixelFormat Format = PixelFormat::Unknown;
    std::uint32_t Width = 1;
    std::uint32_t Height = 1;
    std::uint32_t DepthOrArraySize = 1;
    std::uint32_t MipLevels = 1;
    std::uint32_t SampleCount = 1;
    std::uint32_t BindMask = Bind_None;
    ResourceUsage Usage = ResourceUsage::Default;
};

struct TextureSubresourceData
{
    const void* Data = nullptr;
    std::uint32_t RowPitch = 0;
    std::uint32_t SlicePitch = 0;
};

struct ShaderDesc
{
    ShaderStage Stage = ShaderStage::Vertex;
    const char* EntryPoint = "main";
    const char* SourceCode = "";
    const char* DebugName = "";
};

struct VertexAttributeDesc
{
    const char* SemanticName = "";
    std::uint32_t SemanticIndex = 0;
    PixelFormat Format = PixelFormat::Unknown;
    std::uint32_t Offset = 0;
};

struct RasterizerDesc
{
    FillMode Fill = FillMode::Solid;
    CullMode Cull = CullMode::Back;
    bool FrontCounterClockwise = false;
    bool DepthClipEnabled = true;
};

struct DepthStencilDesc
{
    bool DepthEnabled = true;
    bool DepthWriteEnabled = true;
    CompareOp DepthCompare = CompareOp::LessEqual;
};

struct BlendDesc
{
    bool BlendEnabled = false;
    bool AlphaToCoverageEnabled = false;
};

struct GraphicsPipelineDesc
{
    PrimitiveTopology Topology = PrimitiveTopology::TriangleList;
    RasterizerDesc Rasterizer;
    DepthStencilDesc DepthStencil;
    BlendDesc Blend;
    const VertexAttributeDesc* VertexAttributes = nullptr;
    std::uint32_t VertexAttributeCount = 0;
    PixelFormat ColorFormat = PixelFormat::R8G8B8A8_UNorm;
    PixelFormat DepthFormat = PixelFormat::D24_UNorm_S8_UInt;
    std::uint32_t VertexStride = 0;
    ShaderDesc VertexShader;
    ShaderDesc PixelShader;
    const char* DebugName = "";
};

struct Extent2D
{
    std::uint32_t Width = 0;
    std::uint32_t Height = 0;
};

struct ClearColorValue
{
    float R = 0.0f;
    float G = 0.0f;
    float B = 0.0f;
    float A = 1.0f;
};

struct NativeWindow
{
    void* Handle = nullptr;
};

struct SwapchainDesc
{
    Extent2D Extent;
    NativeWindow Window;
    PixelFormat ColorFormat = PixelFormat::R8G8B8A8_UNorm;
    PixelFormat DepthFormat = PixelFormat::D24_UNorm_S8_UInt;
    std::uint32_t BufferCount = 2;
    bool VSyncEnabled = true;
};
}
