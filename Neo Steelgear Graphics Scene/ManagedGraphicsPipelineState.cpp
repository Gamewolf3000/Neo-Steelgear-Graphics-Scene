#include "ManagedGraphicsPipelineState.h"

#include <fstream>

#include <d3dcompiler.h>

ID3DBlob* ManagedGraphicsPipelineState::LoadCSO(const std::string& filepath)
{
	std::ifstream file(filepath, std::ios::binary);

	if (!file.is_open())
		throw std::runtime_error("Could not open CSO file");

	file.seekg(0, std::ios_base::end);
	size_t size = static_cast<size_t>(file.tellg());
	file.seekg(0, std::ios_base::beg);

	ID3DBlob* toReturn = nullptr;
	HRESULT hr = D3DCreateBlob(size, &toReturn);

	if (FAILED(hr))
		throw std::runtime_error("Could not create blob when loading CSO");

	file.read(static_cast<char*>(toReturn->GetBufferPointer()), size);
	file.close();

	return toReturn;
}

D3D12_ROOT_PARAMETER ManagedGraphicsPipelineState::CreateRootDescriptor(
	const RootBufferBinding& binding)
{
	D3D12_ROOT_PARAMETER toReturn;
	toReturn.ShaderVisibility = binding.shaderAssociation;
	toReturn.ParameterType = binding.parameterType;
	toReturn.Descriptor.ShaderRegister = binding.registerNr;
	toReturn.Descriptor.RegisterSpace = 0;
	return toReturn;
}

void ManagedGraphicsPipelineState::CreateRootSignature(
	std::vector<RootBufferBinding> rootBufferBindings,
	const std::vector<D3D12_STATIC_SAMPLER_DESC>& staticSamplers)
{
	std::vector<D3D12_ROOT_PARAMETER> rootParameters;
	rootParameters.reserve(rootBufferBindings.size());
	for (auto& binding : rootBufferBindings)
		rootParameters.push_back(CreateRootDescriptor(binding));

	D3D12_ROOT_SIGNATURE_DESC desc;
	desc.NumParameters = static_cast<unsigned int>(rootParameters.size());
	desc.pParameters = rootParameters.size() != 0 ? &rootParameters[0] : nullptr;
	desc.NumStaticSamplers = static_cast<UINT>(staticSamplers.size());
	desc.pStaticSamplers = staticSamplers.size() > 0 ? &staticSamplers[0] : nullptr;
	desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;

	ID3DBlob* serialized;
	ID3DBlob* error;
	HRESULT hr = D3D12SerializeRootSignature(&desc,
		D3D_ROOT_SIGNATURE_VERSION_1_0, &serialized, &error);

	if (FAILED(hr))
	{
		std::string blobMessage = 
			std::string(static_cast<char*>(error->GetBufferPointer()));
		throw std::runtime_error("Could not serialize root signature: " + blobMessage);
	}

	hr = device->CreateRootSignature(0, serialized->GetBufferPointer(),
		serialized->GetBufferSize(), IID_PPV_ARGS(&rootSignature));

	if (FAILED(hr))
		throw std::runtime_error("Could not create root signature");
}

void ManagedGraphicsPipelineState::CreatePipelineState(
	const std::array<std::string, 5>& shaderPaths, DXGI_FORMAT dsvFormat,
	const std::vector<DXGI_FORMAT>& rtvFormats)
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc;
	ZeroMemory(&desc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	desc.pRootSignature = rootSignature;
	std::array<D3D12_SHADER_BYTECODE*, 5> shaders = { &desc.VS, &desc.HS,
		&desc.DS, &desc.GS, &desc.PS };
	std::array<D3DPtr<ID3DBlob>, 5> shaderBlobs;

	for (int i = 0; i < 5; ++i)
	{
		if(shaderPaths[i] != "")
		{
			shaderBlobs[i] = LoadCSO(shaderPaths[i]);
			(*shaders[i]).pShaderBytecode = shaderBlobs[i]->GetBufferPointer();
			(*shaders[i]).BytecodeLength = shaderBlobs[i]->GetBufferSize();
		}
	}

	desc.SampleMask = UINT_MAX;
	desc.RasterizerState = CreateRasterizerDesc();
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	desc.NumRenderTargets = static_cast<UINT>(rtvFormats.size());

	desc.BlendState.AlphaToCoverageEnable = false;
	desc.BlendState.IndependentBlendEnable = false;

	for (unsigned int i = 0; i < rtvFormats.size(); ++i)
	{
		desc.RTVFormats[i] = rtvFormats[i];
		desc.BlendState.RenderTarget[i] = CreateBlendDesc();
	}

	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.DSVFormat = dsvFormat;
	desc.DepthStencilState = CreateDepthStencilDesc();
	desc.StreamOutput = CreateStreamOutputDesc();
	desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	HRESULT hr;
	hr = device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pipelineState));

	if (FAILED(hr))
		throw std::runtime_error("Could not create graphics pipeline state");
}

void ManagedGraphicsPipelineState::CreateViewport(unsigned int width,
	unsigned int height)
{
	viewport.Width = static_cast<float>(width);
	viewport.Height = static_cast<float>(height);
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
}

void ManagedGraphicsPipelineState::CreateScissorRect(unsigned int width,
	unsigned int height)
{
	scissorRect.top = 0;
	scissorRect.bottom = height;
	scissorRect.left = 0;
	scissorRect.right = width;
}

D3D12_RASTERIZER_DESC ManagedGraphicsPipelineState::CreateRasterizerDesc()
{
	D3D12_RASTERIZER_DESC toReturn;
	toReturn.FillMode = D3D12_FILL_MODE_SOLID;
	toReturn.CullMode = D3D12_CULL_MODE_BACK;
	toReturn.FrontCounterClockwise = false;
	toReturn.DepthBias = 0;
	toReturn.DepthBiasClamp = 0.0f;
	toReturn.SlopeScaledDepthBias = 0.0f;
	toReturn.DepthClipEnable = true;
	toReturn.MultisampleEnable = false;
	toReturn.AntialiasedLineEnable = false;
	toReturn.ForcedSampleCount = 0;
	toReturn.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	return toReturn;
}

D3D12_RENDER_TARGET_BLEND_DESC ManagedGraphicsPipelineState::CreateBlendDesc()
{
	D3D12_RENDER_TARGET_BLEND_DESC toReturn;
	toReturn.BlendEnable = false;
	toReturn.LogicOpEnable = false;
	toReturn.SrcBlend = D3D12_BLEND_ONE;
	toReturn.DestBlend = D3D12_BLEND_ZERO;
	toReturn.BlendOp = D3D12_BLEND_OP_ADD;
	toReturn.SrcBlendAlpha = D3D12_BLEND_ONE;
	toReturn.DestBlendAlpha = D3D12_BLEND_ZERO;
	toReturn.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	toReturn.LogicOp = D3D12_LOGIC_OP_NOOP;
	toReturn.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	return toReturn;
}

D3D12_DEPTH_STENCIL_DESC ManagedGraphicsPipelineState::CreateDepthStencilDesc()
{
	D3D12_DEPTH_STENCIL_DESC toReturn;
	toReturn.DepthEnable = true;
	toReturn.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	toReturn.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	toReturn.StencilEnable = false;
	toReturn.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	toReturn.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
	toReturn.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	toReturn.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	toReturn.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	toReturn.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	toReturn.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	toReturn.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	toReturn.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	toReturn.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	return toReturn;
}

D3D12_STREAM_OUTPUT_DESC ManagedGraphicsPipelineState::CreateStreamOutputDesc()
{
	D3D12_STREAM_OUTPUT_DESC toReturn;
	toReturn.pSODeclaration = nullptr;
	toReturn.NumEntries = 0;
	toReturn.pBufferStrides = nullptr;
	toReturn.NumStrides = 0;
	toReturn.RasterizedStream = 0;
	return toReturn;
}

void ManagedGraphicsPipelineState::Initialize(ID3D12Device* deviceToUse,
	const GraphicsPipelineData& graphicsPipelineData)
{
	device = deviceToUse;
	CreateRootSignature(graphicsPipelineData.rootBufferBindings,
		graphicsPipelineData.staticSamplers);
	CreatePipelineState(graphicsPipelineData.shaderPaths, 
		graphicsPipelineData.dsvFormat, graphicsPipelineData.rtvFormats);
	CreateViewport(graphicsPipelineData.rendertargetWidth,
		graphicsPipelineData.rendertargetHeight);
	CreateScissorRect(graphicsPipelineData.rendertargetWidth,
		graphicsPipelineData.rendertargetHeight);
}

void ManagedGraphicsPipelineState::SetPipelineState(
	ID3D12GraphicsCommandList * commandList)
{
	commandList->SetGraphicsRootSignature(rootSignature);
	commandList->IASetPrimitiveTopology(topology);
	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &scissorRect);
	commandList->SetPipelineState(pipelineState);
}

void ManagedGraphicsPipelineState::ChangeBackbufferDependent(
	unsigned int newWidth, unsigned int newHeight)
{
	CreateViewport(newWidth, newHeight);
	CreateScissorRect(newWidth, newHeight);
}
