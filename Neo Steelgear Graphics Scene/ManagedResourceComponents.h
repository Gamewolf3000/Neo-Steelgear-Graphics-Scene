#pragma once

#include <cstdint>
#include <optional>

#include "FrameBased.h"
#include "FrameBufferComponent.h"
#include "FrameTexture2DComponent.h"
#include "DirectAccessComponentBinder.h"
#include "ComponentDescriptorHeap.h"
#include "D3DPtr.h"

typedef unsigned int ComponentIndex;

enum class ComponentType
{
	BUFFER,
	TEXTURE1D,
	TEXTURE2D,
	TEXTURE3D
};

struct ComponentIdentifier
{
	//ComponentIndex globalIndex;
	ComponentType type;
	size_t localIndex = 0;
	bool dynamicComponent = true;

	//operator ComponentIndex() const { return globalIndex; }
	bool operator==(const ComponentIdentifier& other) const
	{
		return this->type == other.type && this->localIndex == other.localIndex &&
			this->dynamicComponent == other.dynamicComponent;
	}
};

namespace std {

	template <>
	struct hash<ComponentIdentifier>
	{
		size_t operator()(const ComponentIdentifier& identifier) const
		{
			return ((hash<ComponentType>()(identifier.type)
				^ (hash<size_t>()(identifier.localIndex) << 1)) >> 1)
				^ (hash<bool>()(identifier.dynamicComponent) << 1);
		}
	};

}

template<FrameType Frames>
class ManagedResourceComponents : FrameBased<Frames>
{
private:
	unsigned int rtvSize = 0;
	unsigned int dsvSize = 0;
	unsigned int shaderViewSize = 0;
	unsigned int descriptorsPerFrame = 0;

	ID3D12Device* device = nullptr;
	//D3DPtr<ID3D12DescriptorHeap> shaderVisibleHeap;

	//std::vector<ResourceComponent*> allComponents;
	std::vector<FrameBufferComponent<Frames>> dynamicBufferComponents;
	std::vector<FrameBufferComponent<1>> staticBufferComponents;
	std::vector<FrameTexture2DComponent<Frames>> dynamicTexture2DComponents;
	std::vector<FrameTexture2DComponent<1>> staticTexture2DComponents;

	//std::vector<typename DirectAccessComponentBinder<ComponentIndex,
	//	Frames>::ComponentToBind> bindings;
	//std::vector<ComponentIdentifier> componentIdentifiers;
	//DirectAccessComponentBinder<ComponentIndex, Frames> componentBinder;
	ComponentDescriptorHeap<Frames, ComponentIdentifier> componentDescriptorHeap;

	ResourceUploader uploaders[Frames];

	template<typename ViewDescType>
	DescriptorAllocationInfo<ViewDescType> CreateCustomDAI(ViewType viewType,
		size_t nrOfDescriptors, ViewDescType viewDesc);

	template<typename ViewDescType>
	std::vector<DescriptorAllocationInfo<ViewDescType>> CreateCustomDAIVector(
		std::optional<ViewDescType> cbv, std::optional<ViewDescType> srv,
		std::optional<ViewDescType> uav, std::optional<ViewDescType> rtv,
		std::optional<ViewDescType> dsv, size_t maxNrOfDescriptors);

	template<typename ViewDescType>
	DescriptorAllocationInfo<ViewDescType> CreateDefaultDAI(ViewType viewType,
		size_t nrOfDescriptors);

	template<typename ViewDescType>
	std::vector<DescriptorAllocationInfo<ViewDescType>> CreateDefaultDAIVector(
		bool cbv, bool srv, bool uav, bool rtv, bool dsv, size_t maxNrOfDescriptors);

	//void AddComponentToBindings(ComponentIndex componentIndex,
	//	unsigned int maxElements, ViewType viewType);

	//void AddComponentBindings(ComponentIndex componentIndex,
	//	unsigned int maxElements, bool cbv, bool srv, bool uav);

	void InitialiseResourceUploaders(size_t minSizePerUploader,
		AllocationStrategy allocationStrategy);

public:
	ManagedResourceComponents() = default;
	~ManagedResourceComponents() = default;

	void Initialize(ID3D12Device* deviceToUse, size_t minSizePerUploader,
		AllocationStrategy allocationStrategy);
	void FinalizeComponents();

	template<typename Element>
	ComponentIdentifier CreateBufferComponent(bool dynamic,
		unsigned int maxElements, UpdateType componentUpdateType, bool cbv,
		bool srv, bool uav);

	template<typename Element>
	ComponentIdentifier CreateBufferComponent(bool dynamic,
		unsigned int maxElements, UpdateType componentUpdateType,
		std::optional<BufferViewDesc> cbv, std::optional<BufferViewDesc> srv,
		std::optional<BufferViewDesc> uav);

	ComponentIdentifier CreateTexture2DComponent(bool dynamic,
		size_t totalBytes, unsigned int maxNrOfTextures, std::uint8_t texelSize,
		DXGI_FORMAT texelFormat, UpdateType componentUpdateType,
		bool srv, bool uav = false, bool rtv = false, bool dsv = false);

	ComponentIdentifier CreateTexture2DComponent(bool dynamic,
		size_t totalBytes, unsigned int maxNrOfTextures, std::uint8_t texelSize,
		DXGI_FORMAT texelFormat, UpdateType componentUpdateType,
		std::optional<Texture2DViewDesc> srv, std::optional<Texture2DViewDesc> uav,
		std::optional<Texture2DViewDesc> rtv, std::optional<Texture2DViewDesc> dsv);

	FrameBufferComponent<Frames>& GetDynamicBufferComponent(
		const ComponentIdentifier& componentIdentifier);
	FrameBufferComponent<1>& GetStaticBufferComponent(
		const ComponentIdentifier& componentIdentifier);
	FrameTexture2DComponent<Frames>& GetDynamicTexture2DComponent(
		const ComponentIdentifier& componentIdentifier);
	FrameTexture2DComponent<1>& GetStaticTexture2DComponent(
		const ComponentIdentifier& componentIdentifier);

	void UpdateComponents(ID3D12GraphicsCommandList* commandList);
	void BindComponents(ID3D12GraphicsCommandList* commandList);
	size_t GetComponentDescriptorStart(const ComponentIdentifier& identifier,
		ViewType viewType);
	//void BindComponentIndexBuffer(ID3D12GraphicsCommandList* commandList,
	//	unsigned int rootParameterIndex);

	void SwapFrame() override;
};

template<FrameType Frames>
template<typename ViewDescType>
inline DescriptorAllocationInfo<ViewDescType>
ManagedResourceComponents<Frames>::CreateCustomDAI(ViewType viewType,
	size_t nrOfDescriptors, ViewDescType viewDesc)
{
	DescriptorInfo descriptorInfo;

	if (viewType == ViewType::RTV)
	{
		descriptorInfo.type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		descriptorInfo.descriptorSize = rtvSize;
	}
	else if (viewType == ViewType::DSV)
	{
		descriptorInfo.type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		descriptorInfo.descriptorSize = dsvSize;
	}
	else
	{
		descriptorInfo.type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		descriptorInfo.descriptorSize = shaderViewSize;
	}

	DescriptorAllocationInfo<ViewDescType> toReturn(viewType, descriptorInfo,
		viewDesc, nrOfDescriptors);

	return toReturn;
}

template<FrameType Frames>
template<typename ViewDescType>
inline std::vector<DescriptorAllocationInfo<ViewDescType>>
ManagedResourceComponents<Frames>::CreateCustomDAIVector(
	std::optional<ViewDescType> cbv, std::optional<ViewDescType> srv,
	std::optional<ViewDescType> uav, std::optional<ViewDescType> rtv,
	std::optional<ViewDescType> dsv, size_t maxNrOfDescriptors)
{
	std::vector<DescriptorAllocationInfo<ViewDescType>> toReturn;

	if (cbv.has_value())
	{
		toReturn.push_back(CreateCustomDAI<ViewDescType>(
			ViewType::CBV, maxNrOfDescriptors, cbv.value()));
	}
	if (srv.has_value())
	{
		toReturn.push_back(CreateCustomDAI<ViewDescType>(
			ViewType::SRV, maxNrOfDescriptors, srv.value()));
	}
	if (uav.has_value())
	{
		toReturn.push_back(CreateCustomDAI<ViewDescType>(
			ViewType::UAV, maxNrOfDescriptors, uav.value()));
	}
	if (rtv.has_value())
	{
		toReturn.push_back(CreateCustomDAI<ViewDescType>(
			ViewType::RTV, maxNrOfDescriptors, rtv.value()));
	}
	if (dsv.has_value())
	{
		toReturn.push_back(CreateCustomDAI<ViewDescType>(
			ViewType::DSV, maxNrOfDescriptors, dsv.value()));
	}

	return toReturn;
}

template<FrameType Frames>
template<typename ViewDescType>
inline DescriptorAllocationInfo<ViewDescType>
ManagedResourceComponents<Frames>::CreateDefaultDAI(ViewType viewType,
	size_t nrOfDescriptors)
{
	DescriptorInfo descriptorInfo;

	if (viewType == ViewType::RTV)
	{
		descriptorInfo.type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		descriptorInfo.descriptorSize = rtvSize;
	}
	else if (viewType == ViewType::DSV)
	{
		descriptorInfo.type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		descriptorInfo.descriptorSize = dsvSize;
	}
	else
	{
		descriptorInfo.type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		descriptorInfo.descriptorSize = shaderViewSize;
	}

	DescriptorAllocationInfo<ViewDescType> toReturn(viewType, descriptorInfo,
		ViewDescType(viewType), nrOfDescriptors);

	return toReturn;
}

template<FrameType Frames>
template<typename ViewDescType>
inline std::vector<DescriptorAllocationInfo<ViewDescType>>
ManagedResourceComponents<Frames>::CreateDefaultDAIVector(bool cbv, bool srv,
	bool uav, bool rtv, bool dsv, size_t maxNrOfDescriptors)
{
	std::vector<DescriptorAllocationInfo<ViewDescType>> toReturn;

	if (cbv)
	{
		toReturn.push_back(CreateDefaultDAI<ViewDescType>(
			ViewType::CBV, maxNrOfDescriptors));
	}
	if (srv)
	{
		toReturn.push_back(CreateDefaultDAI<ViewDescType>(
			ViewType::SRV, maxNrOfDescriptors));
	}
	if (uav)
	{
		toReturn.push_back(CreateDefaultDAI<ViewDescType>(
			ViewType::UAV, maxNrOfDescriptors));
	}
	if (rtv)
	{
		toReturn.push_back(CreateDefaultDAI<ViewDescType>(
			ViewType::RTV, maxNrOfDescriptors));
	}
	if (dsv)
	{
		toReturn.push_back(CreateDefaultDAI<ViewDescType>(
			ViewType::DSV, maxNrOfDescriptors));
	}

	return toReturn;
}

template<FrameType Frames>
template<typename Element>
inline ComponentIdentifier
ManagedResourceComponents<Frames>::CreateBufferComponent(bool dynamic,
	unsigned int maxElements, UpdateType componentUpdateType, bool cbv,
	bool srv, bool uav)
{
	BufferInfo bufferInfo;
	bufferInfo.elementSize = sizeof(Element);
	bufferInfo.alignment = alignof(Element);
	ResourceHeapInfo heapInfo(maxElements * sizeof(Element));
	BufferComponentInfo componentInfo{ bufferInfo, componentUpdateType ==
		UpdateType::MAP_UPDATE, heapInfo };

	std::vector<DescriptorAllocationInfo<BufferViewDesc>> descriptorInfo =
		CreateDefaultDAIVector<BufferViewDesc>(cbv, srv, uav, false, false,
			maxElements);

	ComponentIdentifier toReturn;
	if (dynamic)
	{
		dynamicBufferComponents.push_back(FrameBufferComponent<Frames>());
		dynamicBufferComponents.back().Initialize(device, componentUpdateType,
			componentInfo, descriptorInfo);
		//index = static_cast<ComponentIndex>(componentIdentifiers.size());
		toReturn = { ComponentType::BUFFER,
			dynamicBufferComponents.size() - 1, true };
	}
	else
	{
		staticBufferComponents.push_back(FrameBufferComponent<1>());
		staticBufferComponents.back().Initialize(device, componentUpdateType,
			componentInfo, descriptorInfo);
		//index = static_cast<ComponentIndex>(componentIdentifiers.size());
		toReturn = { ComponentType::BUFFER,
			staticBufferComponents.size() - 1, false };
	}

	descriptorsPerFrame += maxElements * 
		static_cast<unsigned int>(descriptorInfo.size());
	//AddComponentBindings(index, maxElements, cbv, srv, uav);

	return toReturn;
}

template<FrameType Frames>
template<typename Element>
inline ComponentIdentifier
ManagedResourceComponents<Frames>::CreateBufferComponent(bool dynamic,
	unsigned int maxElements, UpdateType componentUpdateType,
	std::optional<BufferViewDesc> cbv, std::optional<BufferViewDesc> srv,
	std::optional<BufferViewDesc> uav)
{
	BufferInfo bufferInfo;
	bufferInfo.elementSize = sizeof(Element);
	bufferInfo.alignment = alignof(Element);
	ResourceHeapInfo heapInfo(maxElements * sizeof(Element));
	BufferComponentInfo componentInfo{ bufferInfo, componentUpdateType ==
		UpdateType::MAP_UPDATE, heapInfo };

	std::vector<DescriptorAllocationInfo<BufferViewDesc>> descriptorInfo =
		CreateCustomDAIVector<BufferViewDesc>(cbv, srv, uav, std::nullopt,
			std::nullopt, maxElements);

	ComponentIdentifier toReturn;
	if (dynamic)
	{
		dynamicBufferComponents.push_back(FrameBufferComponent<Frames>());
		dynamicBufferComponents.back().Initialize(device, componentUpdateType,
			componentInfo, descriptorInfo);
		//index = static_cast<ComponentIndex>(componentIdentifiers.size());
		toReturn = { ComponentType::BUFFER,
			dynamicBufferComponents.size() - 1, true };
	}
	else
	{
		staticBufferComponents.push_back(FrameBufferComponent<1>());
		staticBufferComponents.back().Initialize(device, componentUpdateType,
			componentInfo, descriptorInfo);
		//index = static_cast<ComponentIndex>(componentIdentifiers.size());
		toReturn = { ComponentType::BUFFER,
			staticBufferComponents.size() - 1, false };
	}

	descriptorsPerFrame += maxElements * 
		static_cast<unsigned int>(descriptorInfo.size());
	//AddComponentBindings(index, maxElements, cbv.has_value(), srv.has_value(),
	//	uav.has_value());

	return toReturn;
}

//template<FrameType Frames>
//inline void ManagedResourceComponents<Frames>::AddComponentToBindings(
//	ComponentIndex componentIndex, unsigned int maxElements, ViewType viewType)
//{
//	typename DirectAccessComponentBinder<ComponentIndex, 
//		Frames>::ComponentToBind componentToBind;
//	componentToBind.index = componentIndex;
//	componentToBind.maxComponents = maxElements;
//	componentToBind.viewType = viewType;
//	bindings.push_back(componentToBind);
//	descriptorsPerFrame += maxElements;
//}
//
//template<FrameType Frames>
//inline void ManagedResourceComponents<Frames>::AddComponentBindings(
//	ComponentIndex componentIndex, unsigned int maxElements, bool cbv,
//	bool srv, bool uav)
//{
//	if (cbv)
//		AddComponentToBindings(componentIndex, maxElements, ViewType::CBV);
//	if (srv)
//		AddComponentToBindings(componentIndex, maxElements, ViewType::SRV);
//	if (uav)
//		AddComponentToBindings(componentIndex, maxElements, ViewType::UAV);
//}

template<FrameType Frames>
inline void ManagedResourceComponents<Frames>::InitialiseResourceUploaders(
	size_t minSizePerUploader, AllocationStrategy allocationStrategy)
{
	size_t sizePerUploader = 65536 *
		static_cast<size_t>(std::ceil((1.0 * minSizePerUploader) / 65536));

	for (FrameType i = 0; i < Frames; ++i)
		uploaders[i].Initialize(device, sizePerUploader, allocationStrategy);
}

template<FrameType Frames>
inline void
ManagedResourceComponents<Frames>::Initialize(ID3D12Device* deviceToUse,
	size_t minSizePerUploader, AllocationStrategy allocationStrategy)
{
	device = deviceToUse;
	rtvSize = device->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	dsvSize = device->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	shaderViewSize = device->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	InitialiseResourceUploaders(minSizePerUploader, allocationStrategy);
}

template<FrameType Frames>
inline void ManagedResourceComponents<Frames>::FinalizeComponents()
{
	componentDescriptorHeap.Initialize(device, descriptorsPerFrame);
	//allComponents.reserve(componentIdentifiers.size());
	//for (auto& identifier : componentIdentifiers)
	//{
	//	ResourceComponent* toAdd = nullptr;
	//	if (identifier.type == ComponentType::BUFFER &&
	//		identifier.dynamicComponent == true)
	//	{
	//		toAdd = &dynamicBufferComponents[identifier.localIndex];
	//	}
	//	else if (identifier.type == ComponentType::BUFFER &&
	//		identifier.dynamicComponent == false)
	//	{
	//		toAdd = &staticBufferComponents[identifier.localIndex];
	//	}
	//	else if (identifier.type == ComponentType::TEXTURE2D &&
	//		identifier.dynamicComponent == true)
	//	{
	//		toAdd = &dynamicTexture2DComponents[identifier.localIndex];
	//	}
	//	else if (identifier.type == ComponentType::TEXTURE2D &&
	//		identifier.dynamicComponent == false)
	//	{
	//		toAdd = &staticTexture2DComponents[identifier.localIndex];
	//	}

	//	allComponents.push_back(toAdd);
	//}

	//componentBinder.Initialize(device, shaderViewSize, bindings);

	//D3D12_DESCRIPTOR_HEAP_DESC desc;
	//desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	//desc.NumDescriptors = descriptorsPerFrame * Frames;
	//desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	//desc.NodeMask = 0;
	//HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&shaderVisibleHeap));
	//
	//if (FAILED(hr))
	//	throw std::runtime_error("Could not create shader visible heap");
}

template<FrameType Frames>
inline ComponentIdentifier
ManagedResourceComponents<Frames>::CreateTexture2DComponent(bool dynamic,
	size_t totalBytes, unsigned int maxNrOfTextures, std::uint8_t texelSize,
	DXGI_FORMAT texelFormat, UpdateType componentUpdateType,
	bool srv, bool uav, bool rtv, bool dsv)
{
	TextureInfo textureInfo;
	textureInfo.format = texelFormat;
	textureInfo.texelSize = texelSize;
	ResourceHeapInfo heapInfo(totalBytes);
	TextureComponentInfo componentInfo(texelFormat, texelSize,
		componentUpdateType == UpdateType::MAP_UPDATE, heapInfo);

	std::vector<DescriptorAllocationInfo<Texture2DViewDesc>> descriptorInfo =
		CreateDefaultDAIVector<Texture2DViewDesc>(false, srv, uav, rtv, dsv,
			maxNrOfTextures);

	ComponentIdentifier toReturn;
	if (dynamic)
	{
		dynamicTexture2DComponents.push_back(FrameTexture2DComponent<Frames>());
		dynamicTexture2DComponents.back().Initialize(device, componentUpdateType,
			componentInfo, descriptorInfo);
		//index = static_cast<ComponentIndex>(componentIdentifiers.size());
		toReturn = { ComponentType::TEXTURE2D,
			dynamicTexture2DComponents.size() - 1, true };
	}
	else
	{
		staticTexture2DComponents.push_back(FrameTexture2DComponent<1>());
		staticTexture2DComponents.back().Initialize(device, componentUpdateType,
			componentInfo, descriptorInfo);
		//index = static_cast<ComponentIndex>(componentIdentifiers.size());
		toReturn = { ComponentType::TEXTURE2D,
			staticTexture2DComponents.size() - 1, false };
	}

	descriptorsPerFrame += maxNrOfTextures *
		static_cast<unsigned int>(descriptorInfo.size());
	//AddComponentBindings(index, maxNrOfTextures, false, srv, uav);

	return toReturn;
}

template<FrameType Frames>
inline ComponentIdentifier
ManagedResourceComponents<Frames>::CreateTexture2DComponent(bool dynamic,
	size_t totalBytes, unsigned int maxNrOfTextures, std::uint8_t texelSize,
	DXGI_FORMAT texelFormat, UpdateType componentUpdateType,
	std::optional<Texture2DViewDesc> srv, std::optional<Texture2DViewDesc> uav,
	std::optional<Texture2DViewDesc> rtv, std::optional<Texture2DViewDesc> dsv)
{
	TextureInfo textureInfo;
	textureInfo.format = texelFormat;
	textureInfo.texelSize = texelSize;
	ResourceHeapInfo heapInfo(totalBytes);
	TextureComponentInfo componentInfo(texelFormat, texelSize,
		componentUpdateType == UpdateType::MAP_UPDATE, heapInfo);

	std::vector<DescriptorAllocationInfo<Texture2DViewDesc>> descriptorInfo =
		CreateCustomDAIVector<Texture2DViewDesc>(std::nullopt, srv, uav, rtv,
			dsv, maxNrOfTextures);

	ComponentIdentifier toReturn;
	if (dynamic)
	{
		dynamicTexture2DComponents.push_back(FrameTexture2DComponent<Frames>());
		dynamicTexture2DComponents.back().Initialize(device, componentUpdateType,
			componentInfo, descriptorInfo);
		//index = static_cast<ComponentIndex>(componentIdentifiers.size());
		toReturn = { ComponentType::TEXTURE2D,
			dynamicTexture2DComponents.size() - 1, true };
	}
	else
	{
		staticTexture2DComponents.push_back(FrameTexture2DComponent<1>());
		staticTexture2DComponents.back().Initialize(device, componentUpdateType,
			componentInfo, descriptorInfo);
		//index = static_cast<ComponentIndex>(componentIdentifiers.size());
		toReturn = { ComponentType::TEXTURE2D,
			staticTexture2DComponents.size() - 1, false };
	}

	descriptorsPerFrame += maxNrOfTextures *
		static_cast<unsigned int>(descriptorInfo.size());
	//AddComponentBindings(index, maxNrOfTextures, false, srv.has_value(),
	//	uav.has_value());

	return toReturn;
}

template<FrameType Frames>
inline FrameBufferComponent<Frames>&
ManagedResourceComponents<Frames>::GetDynamicBufferComponent(
	const ComponentIdentifier& componentIdentifier)
{
	return dynamicBufferComponents[componentIdentifier.localIndex];
}

template<FrameType Frames>
inline FrameBufferComponent<1>&
ManagedResourceComponents<Frames>::GetStaticBufferComponent(
	const ComponentIdentifier& componentIdentifier)
{
	return staticBufferComponents[componentIdentifier.localIndex];

}

template<FrameType Frames>
inline FrameTexture2DComponent<Frames>&
ManagedResourceComponents<Frames>::GetDynamicTexture2DComponent(
	const ComponentIdentifier& componentIdentifier)
{
	return dynamicTexture2DComponents[componentIdentifier.localIndex];
}

template<FrameType Frames>
inline FrameTexture2DComponent<1>&
ManagedResourceComponents<Frames>::GetStaticTexture2DComponent(
	const ComponentIdentifier& componentIdentifier)
{
	return staticTexture2DComponents[componentIdentifier.localIndex];
}

template<FrameType Frames>
inline void ManagedResourceComponents<Frames>::UpdateComponents(
	ID3D12GraphicsCommandList* commandList)
{
	static std::vector<D3D12_RESOURCE_BARRIER> barriers;

	for (auto& bufferComponent : dynamicBufferComponents)
		bufferComponent.PrepareResourcesForUpdates(barriers);

	for (auto& bufferComponent : staticBufferComponents)
		bufferComponent.PrepareResourcesForUpdates(barriers);

	for (auto& texture2DComponent : dynamicTexture2DComponents)
		texture2DComponent.PrepareResourcesForUpdates(barriers);

	for (auto& texture2DComponent : staticTexture2DComponents)
		texture2DComponent.PrepareResourcesForUpdates(barriers);

	if (barriers.size() != 0)
	{
		commandList->ResourceBarrier(static_cast<UINT>(barriers.size()),
			barriers.data());
		barriers.clear();
	}

	for (auto& bufferComponent : dynamicBufferComponents)
		bufferComponent.PerformUpdates(commandList, uploaders[this->activeFrame]);

	for (auto& bufferComponent : staticBufferComponents)
		bufferComponent.PerformUpdates(commandList, uploaders[this->activeFrame]);

	for (auto& texture2DComponent : dynamicTexture2DComponents)
		texture2DComponent.PerformUpdates(commandList, uploaders[this->activeFrame]);

	for (auto& texture2DComponent : staticTexture2DComponents)
		texture2DComponent.PerformUpdates(commandList, uploaders[this->activeFrame]);
}

template<FrameType Frames>
inline void ManagedResourceComponents<Frames>::BindComponents(
	ID3D12GraphicsCommandList* commandList)
{
	ComponentIdentifier identifier;
	identifier.type = ComponentType::BUFFER;
	identifier.dynamicComponent = true;
	for (size_t i = 0; i < dynamicBufferComponents.size(); ++i)
	{
		identifier.localIndex = i;
		componentDescriptorHeap.AddComponentDescriptors(identifier,
			dynamicBufferComponents[i]);
	}

	identifier.dynamicComponent = false;
	for (size_t i = 0; i < staticBufferComponents.size(); ++i)
	{
		identifier.localIndex = i;
		componentDescriptorHeap.AddComponentDescriptors(identifier,
			staticBufferComponents[i]);
	}

	identifier.type = ComponentType::TEXTURE2D;
	identifier.dynamicComponent = true;
	for (size_t i = 0; i < dynamicTexture2DComponents.size(); ++i)
	{
		identifier.localIndex = i;
		componentDescriptorHeap.AddComponentDescriptors(identifier,
			dynamicTexture2DComponents[i]);
	}

	identifier.dynamicComponent = false;
	for (size_t i = 0; i < staticTexture2DComponents.size(); ++i)
	{
		identifier.localIndex = i;
		componentDescriptorHeap.AddComponentDescriptors(identifier,
			staticTexture2DComponents[i]);
	}

	auto heap = componentDescriptorHeap.GetShaderVisibleHeap();
	commandList->SetDescriptorHeaps(1, &heap);
	//componentBinder.BindComponents(&uploaders[this->activeFrame], commandList,
	//	shaderVisibleHeap, this->activeFrame * descriptorsPerFrame * shaderViewSize,
	//	allComponents);
	//commandList->SetDescriptorHeaps(1, &shaderVisibleHeap);
}

template<FrameType Frames>
inline size_t ManagedResourceComponents<Frames>::GetComponentDescriptorStart(
	const ComponentIdentifier& identifier, ViewType viewType)
{
	return componentDescriptorHeap.GetComponentHeapOffset(identifier, viewType);
}

//template<FrameType Frames>
//inline void ManagedResourceComponents<Frames>::BindComponentIndexBuffer(
//	ID3D12GraphicsCommandList* commandList, unsigned int rootParameterIndex)
//{
//	commandList->SetGraphicsRootConstantBufferView(rootParameterIndex,
//		componentBinder.GetBufferAdress());
//}

template<FrameType Frames>
inline void ManagedResourceComponents<Frames>::SwapFrame()
{
	FrameBased<Frames>::SwapFrame();

	uploaders[this->activeFrame].RestoreUsedMemory();

	for (auto& bufferComponent : dynamicBufferComponents)
		bufferComponent.SwapFrame();

	for (auto& texture2DComponent : dynamicTexture2DComponents)
		texture2DComponent.SwapFrame();

	//componentBinder.SwapFrame();
	componentDescriptorHeap.SwapFrame();
}
