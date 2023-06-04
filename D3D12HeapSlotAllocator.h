#pragma once

#include"D3D12/tools/ToolForD12.h"
#include<list>

class D3D12HeapSlotAllocator
{
public:
	typedef D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHandle;
	typedef decltype(DescriptorHandle::ptr) DescriptorHandleRaw;

	struct HeapSlot
	{
		uint32_t HeapIndex;//index in HeapMap
		D3D12_CPU_DESCRIPTOR_HANDLE Handle;
	};

private:
	struct FreeRange
	{
		DescriptorHandleRaw Start;
		DescriptorHandleRaw End;
	};

	struct HeapEntry
	{
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> Heap = nullptr;
		std::list<D3D12HeapSlotAllocator::FreeRange> FreeList;

		HeapEntry() { }
	};

private:
	ID3D12Device* D3DDevice;

	const D3D12_DESCRIPTOR_HEAP_DESC HeapDesc;

	const uint32_t DescriptorSize;

	std::vector<HeapEntry> HeapMap;

public:
	D3D12HeapSlotAllocator(ID3D12Device* InDevice, D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32_t NumDescriptorsPerHeap);

	~D3D12HeapSlotAllocator();

	HeapSlot AllocateHeapSlot();

	void FreeHeapSlot(const HeapSlot& Slot);

private:
	D3D12_DESCRIPTOR_HEAP_DESC CreateHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32_t NumDescriptorsPerHeap);

	void AllocateHeap();

};

