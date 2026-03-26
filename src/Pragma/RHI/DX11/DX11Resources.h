#pragma once

#include "Pragma/RHI/Resources.h"

#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

namespace Pragma::RHI::DX11
{
class DX11Buffer final : public IBuffer
{
public:
    DX11Buffer(BufferDesc desc, Microsoft::WRL::ComPtr<ID3D11Buffer> buffer, const char* debugName);

    [[nodiscard]] const char* GetDebugName() const noexcept override;
    [[nodiscard]] const BufferDesc& GetDesc() const noexcept override;
    [[nodiscard]] ID3D11Buffer* GetNativeBuffer() const noexcept;

private:
    BufferDesc m_desc;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_buffer;
    const char* m_debugName;
};

class DX11Texture final : public ITexture
{
public:
    DX11Texture(
        TextureDesc desc,
        Microsoft::WRL::ComPtr<ID3D11Texture2D> texture,
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shaderResourceView,
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetView,
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthStencilView,
        const char* debugName);

    [[nodiscard]] const char* GetDebugName() const noexcept override;
    [[nodiscard]] const TextureDesc& GetDesc() const noexcept override;
    [[nodiscard]] ID3D11Texture2D* GetNativeTexture() const noexcept;
    [[nodiscard]] ID3D11ShaderResourceView* GetShaderResourceView() const noexcept;
    [[nodiscard]] ID3D11RenderTargetView* GetRenderTargetView() const noexcept;
    [[nodiscard]] ID3D11DepthStencilView* GetDepthStencilView() const noexcept;

private:
    TextureDesc m_desc;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_texture;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_shaderResourceView;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_renderTargetView;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_depthStencilView;
    const char* m_debugName;
};

class DX11Shader final : public IShader
{
public:
    DX11Shader(
        ShaderDesc desc,
        Microsoft::WRL::ComPtr<ID3DBlob> bytecode,
        Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader,
        Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader);

    [[nodiscard]] const char* GetDebugName() const noexcept override;
    [[nodiscard]] ShaderStage GetStage() const noexcept override;
    [[nodiscard]] const ShaderDesc& GetDesc() const noexcept override;
    [[nodiscard]] ID3DBlob* GetBytecode() const noexcept;
    [[nodiscard]] ID3D11VertexShader* GetVertexShader() const noexcept;
    [[nodiscard]] ID3D11PixelShader* GetPixelShader() const noexcept;

private:
    ShaderDesc m_desc;
    Microsoft::WRL::ComPtr<ID3DBlob> m_bytecode;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;
};

class DX11PipelineState final : public IPipelineState
{
public:
    DX11PipelineState(
        GraphicsPipelineDesc desc,
        Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout,
        Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader,
        Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader,
        Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizerState,
        Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthStencilState,
        Microsoft::WRL::ComPtr<ID3D11BlendState> blendState);

    [[nodiscard]] const char* GetDebugName() const noexcept override;
    [[nodiscard]] const GraphicsPipelineDesc& GetDesc() const noexcept override;
    [[nodiscard]] ID3D11InputLayout* GetInputLayout() const noexcept;
    [[nodiscard]] ID3D11VertexShader* GetVertexShader() const noexcept;
    [[nodiscard]] ID3D11PixelShader* GetPixelShader() const noexcept;
    [[nodiscard]] ID3D11RasterizerState* GetRasterizerState() const noexcept;
    [[nodiscard]] ID3D11DepthStencilState* GetDepthStencilState() const noexcept;
    [[nodiscard]] ID3D11BlendState* GetBlendState() const noexcept;

private:
    GraphicsPipelineDesc m_desc;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_rasterizerState;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_depthStencilState;
    Microsoft::WRL::ComPtr<ID3D11BlendState> m_blendState;
};
}
