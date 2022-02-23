#pragma once

#include <array>
#include <string>
#include <vector>

#include <d3d12.h>

#include "D3DPtr.h"

struct GraphicsPipelineData
{
	std::array<std::string, 5> shaderPaths = {"", "", "", "", ""};
	std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers;
	unsigned int rendertargetWidth;
	unsigned int rendertargetHeight;
	DXGI_FORMAT dsvFormat = DXGI_FORMAT_D32_FLOAT;
	std::vector<DXGI_FORMAT> rtvFormats = { DXGI_FORMAT_R8G8B8A8_UNORM };
};

class ManagedGraphicsPipelineState
{
private:
	D3DPtr<ID3D12RootSignature> rootSignature;
	D3DPtr<ID3D12PipelineState> pipelineState;
	D3D12_VIEWPORT viewport;
	D3D12_RECT scissorRect;
	D3D_PRIMITIVE_TOPOLOGY topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	ID3D12Device* device = nullptr;

	ID3DBlob* LoadCSO(const std::string& filepath);

	void CreateRootSignature(
		const std::vector<D3D12_STATIC_SAMPLER_DESC>& staticSamplers);
	void CreatePipelineState(const std::array<std::string, 5>& shaderPaths,
		DXGI_FORMAT dsvFormat, const std::vector<DXGI_FORMAT>& rtvFormats);
	void CreateViewport(unsigned int width, unsigned int height);
	void CreateScissorRect(unsigned int width, unsigned int height);

	D3D12_RASTERIZER_DESC CreateRasterizerDesc();
	D3D12_RENDER_TARGET_BLEND_DESC CreateBlendDesc();
	D3D12_DEPTH_STENCIL_DESC CreateDepthStencilDesc();
	D3D12_STREAM_OUTPUT_DESC CreateStreamOutputDesc();

public:
	ManagedGraphicsPipelineState() = default;
	~ManagedGraphicsPipelineState() = default;

	void Initialize(ID3D12Device* deviceToUse,
		const GraphicsPipelineData& graphicsPipelineData);

	void SetPipelineState(ID3D12GraphicsCommandList* commandList);
};