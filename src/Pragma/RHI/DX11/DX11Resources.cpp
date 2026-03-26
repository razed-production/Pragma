#include "Pragma/RHI/DX11/DX11Resources.h"

#include <utility>

namespace Pragma::RHI::DX11
{
DX11Buffer::DX11Buffer(BufferDesc desc, Microsoft::WRL::ComPtr<ID3D11Buffer> buffer, const char* debugName)
    : m_desc(desc)
    , m_buffer(std::move(buffer))
    , m_debugName(debugName)
{
}

const char* DX11Buffer::GetDebugName() const noexcept
{
    return m_debugName;
}

const BufferDesc& DX11Buffer::GetDesc() const noexcept
{
    return m_desc;
}

ID3D11Buffer* DX11Buffer::GetNativeBuffer() const noexcept
{
    return m_buffer.Get();
}

DX11Texture::DX11Texture(
    TextureDesc desc,
    Microsoft::WRL::ComPtr<ID3D11Texture2D> texture,
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shaderResourceView,
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetView,
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthStencilView,
    const char* debugName)
    : m_desc(desc)
    , m_texture(std::move(texture))
    , m_shaderResourceView(std::move(shaderResourceView))
    , m_renderTargetView(std::move(renderTargetView))
    , m_depthStencilView(std::move(depthStencilView))
    , m_debugName(debugName)
{
}

const char* DX11Texture::GetDebugName() const noexcept
{
    return m_debugName;
}

const TextureDesc& DX11Texture::GetDesc() const noexcept
{
    return m_desc;
}

ID3D11Texture2D* DX11Texture::GetNativeTexture() const noexcept
{
    return m_texture.Get();
}

ID3D11ShaderResourceView* DX11Texture::GetShaderResourceView() const noexcept
{
    return m_shaderResourceView.Get();
}

ID3D11RenderTargetView* DX11Texture::GetRenderTargetView() const noexcept
{
    return m_renderTargetView.Get();
}

ID3D11DepthStencilView* DX11Texture::GetDepthStencilView() const noexcept
{
    return m_depthStencilView.Get();
}

DX11Shader::DX11Shader(
    ShaderDesc desc,
    Microsoft::WRL::ComPtr<ID3DBlob> bytecode,
    Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader,
    Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader)
    : m_desc(desc)
    , m_bytecode(std::move(bytecode))
    , m_vertexShader(std::move(vertexShader))
    , m_pixelShader(std::move(pixelShader))
{
}

const char* DX11Shader::GetDebugName() const noexcept
{
    return m_desc.DebugName;
}

ShaderStage DX11Shader::GetStage() const noexcept
{
    return m_desc.Stage;
}

const ShaderDesc& DX11Shader::GetDesc() const noexcept
{
    return m_desc;
}

ID3DBlob* DX11Shader::GetBytecode() const noexcept
{
    return m_bytecode.Get();
}

ID3D11VertexShader* DX11Shader::GetVertexShader() const noexcept
{
    return m_vertexShader.Get();
}

ID3D11PixelShader* DX11Shader::GetPixelShader() const noexcept
{
    return m_pixelShader.Get();
}

DX11PipelineState::DX11PipelineState(
    GraphicsPipelineDesc desc,
    Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout,
    Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader,
    Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader,
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizerState,
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthStencilState,
    Microsoft::WRL::ComPtr<ID3D11BlendState> blendState)
    : m_desc(desc)
    , m_inputLayout(std::move(inputLayout))
    , m_vertexShader(std::move(vertexShader))
    , m_pixelShader(std::move(pixelShader))
    , m_rasterizerState(std::move(rasterizerState))
    , m_depthStencilState(std::move(depthStencilState))
    , m_blendState(std::move(blendState))
{
}

const char* DX11PipelineState::GetDebugName() const noexcept
{
    return m_desc.DebugName;
}

const GraphicsPipelineDesc& DX11PipelineState::GetDesc() const noexcept
{
    return m_desc;
}

ID3D11InputLayout* DX11PipelineState::GetInputLayout() const noexcept
{
    return m_inputLayout.Get();
}

ID3D11VertexShader* DX11PipelineState::GetVertexShader() const noexcept
{
    return m_vertexShader.Get();
}

ID3D11PixelShader* DX11PipelineState::GetPixelShader() const noexcept
{
    return m_pixelShader.Get();
}

ID3D11RasterizerState* DX11PipelineState::GetRasterizerState() const noexcept
{
    return m_rasterizerState.Get();
}

ID3D11DepthStencilState* DX11PipelineState::GetDepthStencilState() const noexcept
{
    return m_depthStencilState.Get();
}

ID3D11BlendState* DX11PipelineState::GetBlendState() const noexcept
{
    return m_blendState.Get();
}
}
