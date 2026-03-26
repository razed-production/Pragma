#include "Pragma/RHI/DX11/DX11Device.h"

#include "Pragma/Core/Log.h"
#include "Pragma/Core/Profiler.h"
#include "Pragma/RHI/DX11/DX11CommandList.h"
#include "Pragma/RHI/DX11/DX11Resources.h"
#include "Pragma/RHI/DX11/DX11Swapchain.h"

#include <cstring>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace
{
Pragma::Core::LogLevel ToLogLevel(const D3D11_MESSAGE_SEVERITY severity) noexcept
{
    switch (severity)
    {
    case D3D11_MESSAGE_SEVERITY_WARNING:
        return Pragma::Core::LogLevel::Warning;
    case D3D11_MESSAGE_SEVERITY_ERROR:
    case D3D11_MESSAGE_SEVERITY_CORRUPTION:
        return Pragma::Core::LogLevel::Error;
    case D3D11_MESSAGE_SEVERITY_INFO:
    case D3D11_MESSAGE_SEVERITY_MESSAGE:
    default:
        return Pragma::Core::LogLevel::Info;
    }
}

DXGI_FORMAT ToDxgiFormat(const Pragma::RHI::PixelFormat format)
{
    switch (format)
    {
    case Pragma::RHI::PixelFormat::R8G8B8A8_UNorm:
        return DXGI_FORMAT_R8G8B8A8_UNORM;
    case Pragma::RHI::PixelFormat::R32G32_Float:
        return DXGI_FORMAT_R32G32_FLOAT;
    case Pragma::RHI::PixelFormat::R32G32B32_Float:
        return DXGI_FORMAT_R32G32B32_FLOAT;
    case Pragma::RHI::PixelFormat::R16G16B16A16_Float:
        return DXGI_FORMAT_R16G16B16A16_FLOAT;
    case Pragma::RHI::PixelFormat::D24_UNorm_S8_UInt:
        return DXGI_FORMAT_D24_UNORM_S8_UINT;
    case Pragma::RHI::PixelFormat::D32_Float:
        return DXGI_FORMAT_D32_FLOAT;
    default:
        return DXGI_FORMAT_UNKNOWN;
    }
}

D3D11_USAGE ToD3DUsage(const Pragma::RHI::ResourceUsage usage)
{
    switch (usage)
    {
    case Pragma::RHI::ResourceUsage::Immutable:
        return D3D11_USAGE_IMMUTABLE;
    case Pragma::RHI::ResourceUsage::Dynamic:
        return D3D11_USAGE_DYNAMIC;
    case Pragma::RHI::ResourceUsage::Staging:
        return D3D11_USAGE_STAGING;
    case Pragma::RHI::ResourceUsage::Default:
    default:
        return D3D11_USAGE_DEFAULT;
    }
}

UINT ToD3DBindFlags(const std::uint32_t bindMask)
{
    UINT result = 0;

    if ((bindMask & Pragma::RHI::Bind_VertexBuffer) != 0)
    {
        result |= D3D11_BIND_VERTEX_BUFFER;
    }
    if ((bindMask & Pragma::RHI::Bind_IndexBuffer) != 0)
    {
        result |= D3D11_BIND_INDEX_BUFFER;
    }
    if ((bindMask & Pragma::RHI::Bind_ConstantBuffer) != 0)
    {
        result |= D3D11_BIND_CONSTANT_BUFFER;
    }
    if ((bindMask & Pragma::RHI::Bind_ShaderResource) != 0)
    {
        result |= D3D11_BIND_SHADER_RESOURCE;
    }
    if ((bindMask & Pragma::RHI::Bind_RenderTarget) != 0)
    {
        result |= D3D11_BIND_RENDER_TARGET;
    }
    if ((bindMask & Pragma::RHI::Bind_DepthStencil) != 0)
    {
        result |= D3D11_BIND_DEPTH_STENCIL;
    }
    if ((bindMask & Pragma::RHI::Bind_UnorderedAccess) != 0)
    {
        result |= D3D11_BIND_UNORDERED_ACCESS;
    }

    return result;
}

const char* ToShaderTarget(const Pragma::RHI::ShaderStage stage)
{
    switch (stage)
    {
    case Pragma::RHI::ShaderStage::Vertex:
        return "vs_5_0";
    case Pragma::RHI::ShaderStage::Pixel:
        return "ps_5_0";
    default:
        return "";
    }
}

D3D11_FILL_MODE ToD3DFillMode(const Pragma::RHI::FillMode fillMode)
{
    return fillMode == Pragma::RHI::FillMode::Wireframe ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;
}

D3D11_CULL_MODE ToD3DCullMode(const Pragma::RHI::CullMode cullMode)
{
    switch (cullMode)
    {
    case Pragma::RHI::CullMode::None:
        return D3D11_CULL_NONE;
    case Pragma::RHI::CullMode::Front:
        return D3D11_CULL_FRONT;
    case Pragma::RHI::CullMode::Back:
    default:
        return D3D11_CULL_BACK;
    }
}

D3D11_COMPARISON_FUNC ToD3DCompareOp(const Pragma::RHI::CompareOp compareOp)
{
    switch (compareOp)
    {
    case Pragma::RHI::CompareOp::Never:
        return D3D11_COMPARISON_NEVER;
    case Pragma::RHI::CompareOp::Less:
        return D3D11_COMPARISON_LESS;
    case Pragma::RHI::CompareOp::Equal:
        return D3D11_COMPARISON_EQUAL;
    case Pragma::RHI::CompareOp::LessEqual:
        return D3D11_COMPARISON_LESS_EQUAL;
    case Pragma::RHI::CompareOp::Greater:
        return D3D11_COMPARISON_GREATER;
    case Pragma::RHI::CompareOp::NotEqual:
        return D3D11_COMPARISON_NOT_EQUAL;
    case Pragma::RHI::CompareOp::GreaterEqual:
        return D3D11_COMPARISON_GREATER_EQUAL;
    case Pragma::RHI::CompareOp::Always:
    default:
        return D3D11_COMPARISON_ALWAYS;
    }
}
}

namespace Pragma::RHI::DX11
{
DX11Device::DX11Device()
{
    UINT createFlags = 0;
#if defined(_DEBUG)
    createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    const D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0
    };

    D3D_FEATURE_LEVEL createdFeatureLevel = D3D_FEATURE_LEVEL_11_0;
    auto createDevice = [&](const UINT flags, const D3D_FEATURE_LEVEL* requestedFeatureLevels, const UINT featureLevelCount)
    {
        return D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            flags,
            requestedFeatureLevels,
            featureLevelCount,
            D3D11_SDK_VERSION,
            &m_device,
            &createdFeatureLevel,
            &m_context);
    };

    HRESULT result = createDevice(
        createFlags,
        featureLevels,
        ARRAYSIZE(featureLevels));

    if (result == E_INVALIDARG)
    {
        result = createDevice(createFlags, &featureLevels[1], 1);
    }

#if defined(_DEBUG)
    if (FAILED(result))
    {
        result = createDevice(0, featureLevels, ARRAYSIZE(featureLevels));
        if (result == E_INVALIDARG)
        {
            result = createDevice(0, &featureLevels[1], 1);
        }
    }
#endif

    if (FAILED(result))
    {
        throw std::runtime_error("Failed to create DX11 device.");
    }

    Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
    result = m_device.As(&dxgiDevice);
    if (FAILED(result))
    {
        throw std::runtime_error("Failed to query IDXGIDevice.");
    }

    Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
    result = dxgiDevice->GetAdapter(&adapter);
    if (FAILED(result))
    {
        throw std::runtime_error("Failed to get DXGI adapter.");
    }

    result = adapter->GetParent(IID_PPV_ARGS(&m_factory));
    if (FAILED(result))
    {
        throw std::runtime_error("Failed to get DXGI factory.");
    }

    m_device.As(&m_infoQueue);

    D3D11_SAMPLER_DESC samplerDesc{};
    samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.MaxAnisotropy = 8;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    result = m_device->CreateSamplerState(&samplerDesc, &m_defaultSampler);
    if (FAILED(result))
    {
        throw std::runtime_error("Failed to create DX11 sampler state.");
    }

    D3D11_SAMPLER_DESC shadowSamplerDesc{};
    shadowSamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    shadowSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    shadowSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    shadowSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    shadowSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    result = m_device->CreateSamplerState(&shadowSamplerDesc, &m_shadowSampler);
    if (FAILED(result))
    {
        throw std::runtime_error("Failed to create DX11 shadow sampler state.");
    }

    D3D11_QUERY_DESC disjointQueryDesc{};
    disjointQueryDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;

    D3D11_QUERY_DESC timestampQueryDesc{};
    timestampQueryDesc.Query = D3D11_QUERY_TIMESTAMP;

    m_gpuProfilingEnabled = true;
    for (GpuFrameQueries& frameQueries : m_gpuFrameQueries)
    {
        if (FAILED(m_device->CreateQuery(&disjointQueryDesc, &frameQueries.DisjointQuery)) ||
            FAILED(m_device->CreateQuery(&timestampQueryDesc, &frameQueries.StartTimestampQuery)) ||
            FAILED(m_device->CreateQuery(&timestampQueryDesc, &frameQueries.EndTimestampQuery)))
        {
            m_gpuProfilingEnabled = false;
            break;
        }

        for (GpuFrameQueries::GpuScopeQueries& scopeQueries : frameQueries.ScopeQueries)
        {
            if (FAILED(m_device->CreateQuery(&timestampQueryDesc, &scopeQueries.StartTimestampQuery)) ||
                FAILED(m_device->CreateQuery(&timestampQueryDesc, &scopeQueries.EndTimestampQuery)))
            {
                m_gpuProfilingEnabled = false;
                break;
            }
        }

        if (!m_gpuProfilingEnabled)
        {
            break;
        }
    }

    if (!m_gpuProfilingEnabled)
    {
        for (GpuFrameQueries& frameQueries : m_gpuFrameQueries)
        {
            frameQueries.DisjointQuery.Reset();
            frameQueries.StartTimestampQuery.Reset();
            frameQueries.EndTimestampQuery.Reset();
        for (GpuFrameQueries::GpuScopeQueries& scopeQueries : frameQueries.ScopeQueries)
        {
            scopeQueries.StartTimestampQuery.Reset();
            scopeQueries.EndTimestampQuery.Reset();
            scopeQueries.Name.clear();
            scopeQueries.Used = false;
        }
        frameQueries.ScopeCount = 0;
        frameQueries.ResolveCooldownFrames = 0;
        frameQueries.ActiveScopeIndex = -1;
        frameQueries.CaptureOpen = false;
        frameQueries.ResolvePending = false;
    }

        Pragma::Core::Log(
            Pragma::Core::LogCategory::RHI,
            Pragma::Core::LogLevel::Warning,
            "DX11 GPU profiling queries are unavailable. CPU profiling remains enabled.");
    }

    Pragma::Core::Log(Pragma::Core::LogCategory::RHI, Pragma::Core::LogLevel::Info, "DX11 device created successfully.");
}

BackendType DX11Device::GetBackendType() const noexcept
{
    return BackendType::Direct3D11;
}

std::unique_ptr<ISwapchain> DX11Device::CreateSwapchain(const SwapchainDesc& desc)
{
    auto swapchain = std::make_unique<DX11Swapchain>(*this, desc);
    m_activeSwapchain = swapchain.get();
    return swapchain;
}

std::unique_ptr<ICommandList> DX11Device::CreateCommandList()
{
    return std::make_unique<DX11CommandList>(*this);
}

std::unique_ptr<IBuffer> DX11Device::CreateBuffer(const BufferDesc& desc, const void* initialData)
{
    D3D11_BUFFER_DESC bufferDesc{};
    bufferDesc.ByteWidth = static_cast<UINT>(desc.SizeInBytes);
    bufferDesc.Usage = ToD3DUsage(desc.Usage);
    bufferDesc.BindFlags = ToD3DBindFlags(desc.BindMask);
    bufferDesc.CPUAccessFlags = desc.CpuWritable ? D3D11_CPU_ACCESS_WRITE : 0u;

    D3D11_SUBRESOURCE_DATA subresourceData{};
    subresourceData.pSysMem = initialData;

    const D3D11_SUBRESOURCE_DATA* initialDataPtr = initialData != nullptr ? &subresourceData : nullptr;

    Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;
    HRESULT result = m_device->CreateBuffer(&bufferDesc, initialDataPtr, &buffer);
    if (FAILED(result))
    {
        throw std::runtime_error("Failed to create DX11 buffer.");
    }

    return std::make_unique<DX11Buffer>(desc, std::move(buffer), "DX11Buffer");
}

std::unique_ptr<ITexture> DX11Device::CreateTexture(const TextureDesc& desc, const TextureSubresourceData* initialData)
{
    D3D11_TEXTURE2D_DESC textureDesc{};
    textureDesc.Width = desc.Width;
    textureDesc.Height = desc.Height;
    textureDesc.MipLevels = desc.MipLevels;
    textureDesc.ArraySize = desc.DepthOrArraySize;
    textureDesc.Format = ToDxgiFormat(desc.Format);
    textureDesc.SampleDesc.Count = desc.SampleCount;
    textureDesc.Usage = ToD3DUsage(desc.Usage);
    textureDesc.BindFlags = ToD3DBindFlags(desc.BindMask);

    std::vector<D3D11_SUBRESOURCE_DATA> subresourceData;
    const D3D11_SUBRESOURCE_DATA* initialDataPtr = nullptr;
    if (initialData != nullptr)
    {
        const std::uint32_t subresourceCount = desc.MipLevels * desc.DepthOrArraySize;
        subresourceData.resize(subresourceCount);
        for (std::uint32_t subresourceIndex = 0; subresourceIndex < subresourceCount; ++subresourceIndex)
        {
            subresourceData[subresourceIndex].pSysMem = initialData[subresourceIndex].Data;
            subresourceData[subresourceIndex].SysMemPitch = initialData[subresourceIndex].RowPitch;
            subresourceData[subresourceIndex].SysMemSlicePitch = initialData[subresourceIndex].SlicePitch;
        }
        initialDataPtr = subresourceData.data();
    }

    Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
    HRESULT result = m_device->CreateTexture2D(&textureDesc, initialDataPtr, &texture);
    if (FAILED(result))
    {
        throw std::runtime_error("Failed to create DX11 texture.");
    }

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shaderResourceView;
    if ((desc.BindMask & Bind_ShaderResource) != 0)
    {
        result = m_device->CreateShaderResourceView(texture.Get(), nullptr, &shaderResourceView);
        if (FAILED(result))
        {
            throw std::runtime_error("Failed to create DX11 shader resource view.");
        }
    }

    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetView;
    if ((desc.BindMask & Bind_RenderTarget) != 0)
    {
        result = m_device->CreateRenderTargetView(texture.Get(), nullptr, &renderTargetView);
        if (FAILED(result))
        {
            throw std::runtime_error("Failed to create DX11 render target view.");
        }
    }

    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthStencilView;
    if ((desc.BindMask & Bind_DepthStencil) != 0)
    {
        result = m_device->CreateDepthStencilView(texture.Get(), nullptr, &depthStencilView);
        if (FAILED(result))
        {
            throw std::runtime_error("Failed to create DX11 depth stencil view.");
        }
    }

    return std::make_unique<DX11Texture>(
        desc,
        std::move(texture),
        std::move(shaderResourceView),
        std::move(renderTargetView),
        std::move(depthStencilView),
        "DX11Texture");
}

std::unique_ptr<IShader> DX11Device::CreateShader(const ShaderDesc& desc)
{
    Microsoft::WRL::ComPtr<ID3DBlob> bytecode = CompileShader(desc);
    Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;

    HRESULT result = S_OK;
    if (desc.Stage == ShaderStage::Vertex)
    {
        result = m_device->CreateVertexShader(bytecode->GetBufferPointer(), bytecode->GetBufferSize(), nullptr, &vertexShader);
    }
    else if (desc.Stage == ShaderStage::Pixel)
    {
        result = m_device->CreatePixelShader(bytecode->GetBufferPointer(), bytecode->GetBufferSize(), nullptr, &pixelShader);
    }

    if (FAILED(result))
    {
        throw std::runtime_error("Failed to create DX11 shader.");
    }

    return std::make_unique<DX11Shader>(desc, std::move(bytecode), std::move(vertexShader), std::move(pixelShader));
}

std::unique_ptr<IPipelineState> DX11Device::CreateGraphicsPipeline(const GraphicsPipelineDesc& desc)
{
    auto vertexShaderObject = CreateShader(desc.VertexShader);
    auto pixelShaderObject = CreateShader(desc.PixelShader);

    auto& dxVertexShader = dynamic_cast<DX11Shader&>(*vertexShaderObject);
    auto& dxPixelShader = dynamic_cast<DX11Shader&>(*pixelShaderObject);

    std::vector<D3D11_INPUT_ELEMENT_DESC> inputElements;
    inputElements.reserve(desc.VertexAttributeCount);

    for (std::uint32_t i = 0; i < desc.VertexAttributeCount; ++i)
    {
        const VertexAttributeDesc& attribute = desc.VertexAttributes[i];
        D3D11_INPUT_ELEMENT_DESC inputElement{};
        inputElement.SemanticName = attribute.SemanticName;
        inputElement.SemanticIndex = attribute.SemanticIndex;
        inputElement.Format = ToDxgiFormat(attribute.Format);
        inputElement.InputSlot = 0;
        inputElement.AlignedByteOffset = attribute.Offset;
        inputElement.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
        inputElement.InstanceDataStepRate = 0;
        inputElements.push_back(inputElement);
    }

    Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;
    HRESULT result = S_OK;
    if (!inputElements.empty())
    {
        result = m_device->CreateInputLayout(
            inputElements.data(),
            static_cast<UINT>(inputElements.size()),
            dxVertexShader.GetBytecode()->GetBufferPointer(),
            dxVertexShader.GetBytecode()->GetBufferSize(),
            &inputLayout);

        if (FAILED(result))
        {
            throw std::runtime_error("Failed to create DX11 input layout.");
        }
    }

    D3D11_RASTERIZER_DESC rasterizerDesc{};
    rasterizerDesc.FillMode = ToD3DFillMode(desc.Rasterizer.Fill);
    rasterizerDesc.CullMode = ToD3DCullMode(desc.Rasterizer.Cull);
    rasterizerDesc.FrontCounterClockwise = desc.Rasterizer.FrontCounterClockwise;
    rasterizerDesc.DepthClipEnable = desc.Rasterizer.DepthClipEnabled;

    Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizerState;
    result = m_device->CreateRasterizerState(&rasterizerDesc, &rasterizerState);
    if (FAILED(result))
    {
        throw std::runtime_error("Failed to create DX11 rasterizer state.");
    }

    D3D11_DEPTH_STENCIL_DESC depthStencilDesc{};
    depthStencilDesc.DepthEnable = desc.DepthStencil.DepthEnabled;
    depthStencilDesc.DepthWriteMask = desc.DepthStencil.DepthWriteEnabled ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
    depthStencilDesc.DepthFunc = ToD3DCompareOp(desc.DepthStencil.DepthCompare);

    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthStencilState;
    result = m_device->CreateDepthStencilState(&depthStencilDesc, &depthStencilState);
    if (FAILED(result))
    {
        throw std::runtime_error("Failed to create DX11 depth stencil state.");
    }

    D3D11_BLEND_DESC blendDesc{};
    blendDesc.AlphaToCoverageEnable = desc.Blend.AlphaToCoverageEnabled;
    blendDesc.RenderTarget[0].BlendEnable = desc.Blend.BlendEnabled;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    Microsoft::WRL::ComPtr<ID3D11BlendState> blendState;
    result = m_device->CreateBlendState(&blendDesc, &blendState);
    if (FAILED(result))
    {
        throw std::runtime_error("Failed to create DX11 blend state.");
    }

    return std::make_unique<DX11PipelineState>(
        desc,
        std::move(inputLayout),
        dxVertexShader.GetVertexShader(),
        dxPixelShader.GetPixelShader(),
        std::move(rasterizerState),
        std::move(depthStencilState),
        std::move(blendState));
}

void DX11Device::UpdateBuffer(IBuffer& buffer, const void* data, const std::uint64_t sizeInBytes)
{
    auto& dxBuffer = dynamic_cast<DX11Buffer&>(buffer);

    if (data == nullptr || sizeInBytes > dxBuffer.GetDesc().SizeInBytes)
    {
        throw std::runtime_error("Invalid DX11 buffer update request.");
    }

    D3D11_MAPPED_SUBRESOURCE mappedResource{};
    HRESULT result = m_context->Map(dxBuffer.GetNativeBuffer(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (FAILED(result))
    {
        throw std::runtime_error("Failed to map DX11 buffer for update.");
    }

    std::memcpy(mappedResource.pData, data, static_cast<std::size_t>(sizeInBytes));
    m_context->Unmap(dxBuffer.GetNativeBuffer(), 0);
}

void DX11Device::Submit(ICommandList&)
{
    if (m_activeGpuFrameQueryIndex < 0)
    {
        ResolveGpuFrameProfiles();
    }
    DrainDebugMessages();
}

ID3D11Device* DX11Device::GetNativeDevice() const noexcept
{
    return m_device.Get();
}

ID3D11DeviceContext* DX11Device::GetImmediateContext() const noexcept
{
    return m_context.Get();
}

ID3D11SamplerState* DX11Device::GetDefaultSampler() const noexcept
{
    return m_defaultSampler.Get();
}

ID3D11SamplerState* DX11Device::GetShadowSampler() const noexcept
{
    return m_shadowSampler.Get();
}

IDXGIFactory2* DX11Device::GetFactory() const noexcept
{
    return m_factory.Get();
}

DX11Swapchain* DX11Device::GetActiveSwapchain() const noexcept
{
    return m_activeSwapchain;
}

void DX11Device::SetActiveSwapchain(DX11Swapchain* swapchain) noexcept
{
    m_activeSwapchain = swapchain;
}

void DX11Device::BeginGpuFrameProfile(const std::uint64_t frameIndex)
{
    if (!m_gpuProfilingEnabled || m_gpuBaselineCaptured)
    {
        return;
    }

    ResolveGpuFrameProfiles();

    if (m_gpuBaselineCaptured)
    {
        return;
    }

    for (const GpuFrameQueries& existingFrameQueries : m_gpuFrameQueries)
    {
        if (existingFrameQueries.CaptureOpen || existingFrameQueries.ResolvePending)
        {
            return;
        }
    }

    GpuFrameQueries& frameQueries = m_gpuFrameQueries[m_gpuFrameWriteIndex];

    frameQueries.FrameIndex = frameIndex;
    frameQueries.CaptureOpen = true;
    frameQueries.ResolvePending = false;
    frameQueries.ScopeCount = 0;
    frameQueries.ResolveCooldownFrames = 0;
    frameQueries.ActiveScopeIndex = -1;
    for (GpuFrameQueries::GpuScopeQueries& scopeQueries : frameQueries.ScopeQueries)
    {
        scopeQueries.Name.clear();
        scopeQueries.Used = false;
    }
    m_activeGpuFrameQueryIndex = static_cast<std::int32_t>(m_gpuFrameWriteIndex);

    m_context->Begin(frameQueries.DisjointQuery.Get());
    m_context->End(frameQueries.StartTimestampQuery.Get());
}

void DX11Device::EndGpuFrameProfile()
{
    if (!m_gpuProfilingEnabled || m_activeGpuFrameQueryIndex < 0)
    {
        return;
    }

    GpuFrameQueries& frameQueries = m_gpuFrameQueries[static_cast<std::size_t>(m_activeGpuFrameQueryIndex)];
    if (frameQueries.ActiveScopeIndex >= 0)
    {
        GpuFrameQueries::GpuScopeQueries& activeScope = frameQueries.ScopeQueries[static_cast<std::size_t>(frameQueries.ActiveScopeIndex)];
        m_context->End(activeScope.EndTimestampQuery.Get());
        frameQueries.ActiveScopeIndex = -1;
    }

    m_context->End(frameQueries.EndTimestampQuery.Get());
    m_context->End(frameQueries.DisjointQuery.Get());
    m_context->Flush();
    frameQueries.CaptureOpen = false;
    frameQueries.ResolvePending = true;
    frameQueries.ResolveCooldownFrames = 2;

    m_gpuFrameWriteIndex = (m_gpuFrameWriteIndex + 1) % m_gpuFrameQueries.size();
    m_activeGpuFrameQueryIndex = -1;
}

void DX11Device::BeginGpuScope(const std::string_view name)
{
    if (!m_gpuProfilingEnabled || m_activeGpuFrameQueryIndex < 0)
    {
        return;
    }

    GpuFrameQueries& frameQueries = m_gpuFrameQueries[static_cast<std::size_t>(m_activeGpuFrameQueryIndex)];
    if (frameQueries.ActiveScopeIndex >= 0 || frameQueries.ScopeCount >= frameQueries.ScopeQueries.size())
    {
        return;
    }

    GpuFrameQueries::GpuScopeQueries& scopeQueries = frameQueries.ScopeQueries[frameQueries.ScopeCount];
    scopeQueries.Name.assign(name.data(), name.size());
    scopeQueries.Used = true;
    frameQueries.ActiveScopeIndex = static_cast<std::int32_t>(frameQueries.ScopeCount);
    ++frameQueries.ScopeCount;
    m_context->End(scopeQueries.StartTimestampQuery.Get());
}

void DX11Device::EndGpuScope()
{
    if (!m_gpuProfilingEnabled || m_activeGpuFrameQueryIndex < 0)
    {
        return;
    }

    GpuFrameQueries& frameQueries = m_gpuFrameQueries[static_cast<std::size_t>(m_activeGpuFrameQueryIndex)];
    if (frameQueries.ActiveScopeIndex < 0)
    {
        return;
    }

    GpuFrameQueries::GpuScopeQueries& scopeQueries = frameQueries.ScopeQueries[static_cast<std::size_t>(frameQueries.ActiveScopeIndex)];
    m_context->End(scopeQueries.EndTimestampQuery.Get());
    frameQueries.ActiveScopeIndex = -1;
}

Microsoft::WRL::ComPtr<ID3DBlob> DX11Device::CompileShader(const ShaderDesc& desc) const
{
    if (desc.SourceCode == nullptr || desc.SourceCode[0] == '\0')
    {
        throw std::runtime_error("Shader source code is empty.");
    }

    UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(_DEBUG)
    compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    Microsoft::WRL::ComPtr<ID3DBlob> bytecode;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
    HRESULT result = D3DCompile(
        desc.SourceCode,
        std::strlen(desc.SourceCode),
        desc.DebugName,
        nullptr,
        nullptr,
        desc.EntryPoint,
        ToShaderTarget(desc.Stage),
        compileFlags,
        0,
        &bytecode,
        &errorBlob);

    if (FAILED(result))
    {
        throw std::runtime_error(errorBlob != nullptr ? static_cast<const char*>(errorBlob->GetBufferPointer()) : "Failed to compile DX11 shader.");
    }

    return bytecode;
}

void DX11Device::DrainDebugMessages()
{
    if (m_infoQueue == nullptr)
    {
        return;
    }

    const unsigned long long messageCount = m_infoQueue->GetNumStoredMessagesAllowedByRetrievalFilter();
    for (unsigned long long messageIndex = m_lastDebugMessageCount; messageIndex < messageCount; ++messageIndex)
    {
        SIZE_T messageLength = 0;
        if (FAILED(m_infoQueue->GetMessage(messageIndex, nullptr, &messageLength)))
        {
            continue;
        }

        std::vector<char> messageBytes(messageLength);
        auto* message = reinterpret_cast<D3D11_MESSAGE*>(messageBytes.data());
        if (FAILED(m_infoQueue->GetMessage(messageIndex, message, &messageLength)))
        {
            continue;
        }

        Pragma::Core::Log(
            Pragma::Core::LogCategory::RHI,
            ToLogLevel(message->Severity),
            std::string("DX11: ") + message->pDescription);
    }

    m_lastDebugMessageCount = messageCount;
}

void DX11Device::ResolveGpuFrameProfiles()
{
    if (!m_gpuProfilingEnabled)
    {
        return;
    }

    for (GpuFrameQueries& frameQueries : m_gpuFrameQueries)
    {
        if (!frameQueries.ResolvePending || frameQueries.CaptureOpen)
        {
            continue;
        }

        if (frameQueries.ResolveCooldownFrames > 0)
        {
            --frameQueries.ResolveCooldownFrames;
            continue;
        }

        D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjointData{};
        const HRESULT disjointResult = m_context->GetData(
            frameQueries.DisjointQuery.Get(),
            &disjointData,
            sizeof(disjointData),
            0);
        if (disjointResult != S_OK)
        {
            continue;
        }

        UINT64 startTimestamp = 0;
        const HRESULT startResult = m_context->GetData(
            frameQueries.StartTimestampQuery.Get(),
            &startTimestamp,
            sizeof(startTimestamp),
            0);
        if (startResult != S_OK)
        {
            continue;
        }

        UINT64 endTimestamp = 0;
        const HRESULT endResult = m_context->GetData(
            frameQueries.EndTimestampQuery.Get(),
            &endTimestamp,
            sizeof(endTimestamp),
            0);
        if (endResult != S_OK)
        {
            continue;
        }

        if (disjointData.Disjoint || disjointData.Frequency == 0 || endTimestamp < startTimestamp)
        {
            frameQueries.CaptureOpen = false;
            frameQueries.ResolvePending = false;
            m_gpuProfilingSaturatedWarningLogged = false;
            continue;
        }

        std::vector<Pragma::Core::GpuProfileEvent> scopeEvents;
        scopeEvents.reserve(frameQueries.ScopeCount);
        bool allScopeQueriesReady = true;
        for (std::uint32_t scopeIndex = 0; scopeIndex < frameQueries.ScopeCount; ++scopeIndex)
        {
            const GpuFrameQueries::GpuScopeQueries& scopeQueries = frameQueries.ScopeQueries[scopeIndex];
            if (!scopeQueries.Used || scopeQueries.Name.empty())
            {
                continue;
            }

            UINT64 scopeStartTimestamp = 0;
            const HRESULT scopeStartResult = m_context->GetData(
                scopeQueries.StartTimestampQuery.Get(),
                &scopeStartTimestamp,
                sizeof(scopeStartTimestamp),
                0);
            if (scopeStartResult != S_OK)
            {
                allScopeQueriesReady = false;
                break;
            }

            UINT64 scopeEndTimestamp = 0;
            const HRESULT scopeEndResult = m_context->GetData(
                scopeQueries.EndTimestampQuery.Get(),
                &scopeEndTimestamp,
                sizeof(scopeEndTimestamp),
                0);
            if (scopeEndResult != S_OK || scopeEndTimestamp < scopeStartTimestamp)
            {
                allScopeQueriesReady = false;
                break;
            }

            const double scopeMilliseconds =
                static_cast<double>(scopeEndTimestamp - scopeStartTimestamp) * 1000.0 / static_cast<double>(disjointData.Frequency);
            scopeEvents.push_back({ scopeQueries.Name, scopeMilliseconds });
        }

        if (!allScopeQueriesReady)
        {
            continue;
        }

        frameQueries.CaptureOpen = false;
        frameQueries.ResolvePending = false;
        m_gpuProfilingSaturatedWarningLogged = false;

        Pragma::Core::GpuFrameProfile gpuFrameProfile{};
        gpuFrameProfile.FrameIndex = frameQueries.FrameIndex;
        gpuFrameProfile.TotalMilliseconds =
            static_cast<double>(endTimestamp - startTimestamp) * 1000.0 / static_cast<double>(disjointData.Frequency);
        gpuFrameProfile.IsValid = true;
        gpuFrameProfile.Events.push_back({ "Frame GPU", gpuFrameProfile.TotalMilliseconds });
        for (const Pragma::Core::GpuProfileEvent& scopeEvent : scopeEvents)
        {
            gpuFrameProfile.Events.push_back(scopeEvent);
        }
        Pragma::Core::SubmitGpuFrameProfile(gpuFrameProfile);

        static bool hasLoggedFirstGpuFrameProfile = false;
        if (!hasLoggedFirstGpuFrameProfile)
        {
            hasLoggedFirstGpuFrameProfile = true;
            m_gpuBaselineCaptured = true;
            std::ostringstream message;
            message << "GPU stats baseline: total=" << gpuFrameProfile.TotalMilliseconds << " ms";
            for (std::size_t eventIndex = 1; eventIndex < gpuFrameProfile.Events.size(); ++eventIndex)
            {
                message << ", " << gpuFrameProfile.Events[eventIndex].Name << "=" << gpuFrameProfile.Events[eventIndex].DurationMilliseconds << " ms";
            }

            Pragma::Core::Log(
                Pragma::Core::LogCategory::RHI,
                Pragma::Core::LogLevel::Info,
                message.str());
        }
    }
}
}
