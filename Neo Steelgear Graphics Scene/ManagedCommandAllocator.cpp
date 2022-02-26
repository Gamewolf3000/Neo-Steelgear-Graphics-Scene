#include "ManagedCommandAllocator.h"

#include <stdexcept>

ManagedCommandAllocator::~ManagedCommandAllocator()
{
	for (auto list : commandLists)
		list->Release();
}

void ManagedCommandAllocator::Initialize(ID3D12Device* deviceToUse, 
	D3D12_COMMAND_LIST_TYPE typeOfList)
{
	device = deviceToUse;
	type = typeOfList;
	device->CreateCommandAllocator(type, IID_PPV_ARGS(&allocator));
	ID3D12GraphicsCommandList* initial;
	device->CreateCommandList(0, type, allocator, nullptr, IID_PPV_ARGS(&initial));
	initial->Close();
	commandLists.push_back(initial);
	currentList = 0;
	firstUnexecuted = 0;
}

ID3D12GraphicsCommandList* ManagedCommandAllocator::ActiveList()
{
	return commandLists[currentList];
}

void ManagedCommandAllocator::FinishActiveList(bool prepareNewList)
{
	HRESULT hr = commandLists[currentList]->Close();
	if (FAILED(hr))
		throw std::runtime_error("Could not close command list");

	++currentList;

	if (prepareNewList)
	{
		if (currentList == commandLists.capacity())
		{
			ID3D12GraphicsCommandList* newList;
			device->CreateCommandList(0, type, allocator, nullptr,
				IID_PPV_ARGS(&newList));
			commandLists.push_back(newList);
		}
		else
		{
			commandLists[currentList]->Reset(allocator, nullptr);
		}
	}

}

void ManagedCommandAllocator::ExecuteCommands(ID3D12CommandQueue* queue)
{
	ID3D12CommandList** listsToExecute;
	listsToExecute = 
		reinterpret_cast<ID3D12CommandList**>(&commandLists[firstUnexecuted]);
	queue->ExecuteCommandLists(currentList - firstUnexecuted, listsToExecute);
	firstUnexecuted = currentList;
}

void ManagedCommandAllocator::Reset()
{
	HRESULT hr = allocator->Reset();
	if (FAILED(hr))
		throw std::runtime_error("Could not reset allocator");

	hr = commandLists[0]->Reset(allocator, nullptr);
	if (FAILED(hr))
		throw std::runtime_error("Could not reset command list");

	currentList = 0;
	firstUnexecuted = 0;
}

size_t ManagedCommandAllocator::GetNrOfStoredLists()
{
	return commandLists.size();
}

ID3D12GraphicsCommandList* ManagedCommandAllocator::GetStoredList(size_t index)
{
	return commandLists[index];
}
