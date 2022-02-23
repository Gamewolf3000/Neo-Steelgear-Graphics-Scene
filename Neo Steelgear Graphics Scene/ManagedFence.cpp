#include "ManagedFence.h"

#include <stdexcept>

ManagedFence::~ManagedFence()
{
	fence->Release();
}

void ManagedFence::Initialize(ID3D12Device* device, size_t initialValue)
{
	currentValue = initialValue;
	device->CreateFence(currentValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	fenceHandle = CreateEventEx(nullptr, 0, 0, EVENT_ALL_ACCESS);
}

void ManagedFence::Signal(ID3D12CommandQueue* queue)
{
	queue->Signal(fence, ++currentValue);
}

void ManagedFence::WaitGPU(ID3D12CommandQueue* queue)
{
	queue->Wait(fence, currentValue);
}

void ManagedFence::WaitCPU()
{
	if (fence->GetCompletedValue() < 1)
	{
		HRESULT hr = fence->SetEventOnCompletion(1, fenceHandle);
		if(FAILED(hr))
			throw std::runtime_error("Could not set wait event for fence");
		WaitForSingleObject(fenceHandle, INFINITE);
	}
}

bool ManagedFence::Completed()
{
	return fence->GetCompletedValue() == currentValue;
}
