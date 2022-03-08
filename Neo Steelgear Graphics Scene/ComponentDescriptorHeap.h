#pragma once

#include <d3d12.h>
#include <unordered_map>
#include <stdexcept>

#include "FrameBased.h"
#include "ResourceComponent.h"

template<FrameType Frames, typename IdentifierType>
class ComponentDescriptorHeap : public FrameBased<Frames>
{
private:
	struct ComponentOffset
	{
		size_t cbvOffset = size_t(-1);
		size_t srvOffset = size_t(-1);
		size_t uavOffset = size_t(-1);
	};

	std::unordered_map<IdentifierType, ComponentOffset> componentOffsets;
	ID3D12Device* device;
	ID3D12DescriptorHeap* cpuHeap = nullptr;
	ID3D12DescriptorHeap* gpuHeap = nullptr;
	size_t descriptorsPerFrame = 0;
	size_t currentOffset = 0;
	unsigned int descriptorSize = 0;

	void CreateDescriptorHeaps();

public:
	ComponentDescriptorHeap() = default;
	~ComponentDescriptorHeap() = default;
	ComponentDescriptorHeap(
		const ComponentDescriptorHeap& other) = delete;
	ComponentDescriptorHeap& operator=(
		const ComponentDescriptorHeap& other) = delete;
	ComponentDescriptorHeap(
		ComponentDescriptorHeap&& other);
	ComponentDescriptorHeap& operator=(
		ComponentDescriptorHeap&& other);

	void Initialize(ID3D12Device* deviceToUse, 
		size_t maxDescriptorsPerFrame);

	size_t AddComponentDescriptors(const ResourceComponent& component);
	size_t GetComponentHeapOffset(const IdentifierType& identifier,
		ViewType viewType);

	void UploadCurrentFrameHeap();
	ID3D12DescriptorHeap* GetShaderVisibleHeap();

	void SwapFrame() override;
};

template<FrameType Frames, typename IdentifierType>
inline void ComponentDescriptorHeap<Frames, IdentifierType>::CreateDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC desc;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.NumDescriptors = descriptorsPerFrame * Frames;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	desc.NodeMask = 0;
	device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&cpuHeap));

	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&gpuHeap));
}

template<FrameType Frames, typename IdentifierType>
inline void ComponentDescriptorHeap<Frames, IdentifierType>::Initialize(
	ID3D12Device* deviceToUse, size_t maxDescriptorsPerFrame)
{
	device = deviceToUse;
	descriptorsPerFrame = maxDescriptorsPerFrame;
	descriptorSize = device->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CreateDescriptorHeaps();
}

template<FrameType Frames, typename IdentifierType>
inline size_t ComponentDescriptorHeap<Frames, IdentifierType>::AddComponentDescriptors(const ResourceComponent& component)
{
	component.
	return size_t();
}

template<FrameType Frames, typename IdentifierType>
inline size_t ComponentDescriptorHeap<Frames, IdentifierType>::GetComponentHeapOffset(
	const IdentifierType& identifier, ViewType viewType)
{
	auto& offsets = componentOffsets[identifier];

	switch (switch_on)
	{
	case ViewType::CBV:
		return offsets.cbvOffset;
	case ViewType::SRV:
		return offsets.srvOffset;
	case ViewType::UAV:
		return offsets.uavOffset;
	default:
		throw std::runtime_error("Attempting to get heap offset of incorrect type")
	}
}

template<FrameType Frames, typename IdentifierType>
inline void ComponentDescriptorHeap<Frames, IdentifierType>::UploadCurrentFrameHeap()
{
	auto destination = gpuHeap->GetCPUDescriptorHandleForHeapStart();
	destination.ptr += activeFrame * descriptorsPerFrame * descriptorSize;
	auto source = cpuHeap->GetCPUDescriptorHandleForHeapStart();
	source.ptr += activeFrame * descriptorsPerFrame * descriptorSize;
	device->CopyDescriptorsSimple(descriptorsPerFrame, destination, source,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

template<FrameType Frames, typename IdentifierType>
inline ID3D12DescriptorHeap* ComponentDescriptorHeap<Frames, IdentifierType>::GetShaderVisibleHeap()
{
	return gpuHeap;
}
