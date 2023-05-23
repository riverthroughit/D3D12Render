#pragma once
#include "D3D12Resource.h"
#include<set>

//基于伙伴系统的显存分配
class D3D12BuddyAllocator
{
public:
	enum class EAllocationStrategy
	{
		PlacedResource,
		ManualSubAllocation
	};

	struct TAllocatorInitData
	{
		EAllocationStrategy AllocationStrategy;

		D3D12_HEAP_TYPE HeapType;

		D3D12_HEAP_FLAGS HeapFlags = D3D12_HEAP_FLAG_NONE;  // Only for PlacedResource

		D3D12_RESOURCE_FLAGS ResourceFlags = D3D12_RESOURCE_FLAG_NONE;  // Only for ManualSubAllocation
	};

public:
	D3D12BuddyAllocator(ID3D12Device* InDevice, const TAllocatorInitData& InInitData);

	~D3D12BuddyAllocator();

	bool AllocResource(uint32_t Size, uint32_t Alignment, D3D12ResourceLocation& ResourceLocation);

	void Deallocate(D3D12ResourceLocation& ResourceLocation);

	void CleanUpAllocations();

	ID3D12Heap* GetBackingHeap() { return BackingHeap; }

	EAllocationStrategy GetAllocationStrategy() { return InitData.AllocationStrategy; }

private:
	void Initialize();

	uint32_t AllocateBlock(uint32_t Order);

	uint32_t GetSizeToAllocate(uint32_t Size, uint32_t Alignment);

	bool CanAllocate(uint32_t SizeToAllocate);

	uint32_t SizeToUnitSize(uint32_t Size) const
	{
		return (Size + (MinBlockSize - 1)) / MinBlockSize;
	}

	uint32_t UnitSizeToOrder(uint32_t Size) const
	{
		unsigned long Result;
		_BitScanReverse(&Result, Size + Size - 1); // ceil(log2(size))
		return Result;
	}

	uint32_t OrderToUnitSize(uint32_t Order) const
	{
		return ((uint32_t)1) << Order;
	}

	void DeallocateInternal(const D3D12BuddyBlockData& Block);

	void DeallocateBlock(uint32_t Offset, uint32_t Order);

	uint32_t GetBuddyOffset(const uint32_t& Offset, const uint32_t& Size)
	{
		return Offset ^ Size;
	}

	uint32_t GetAllocOffsetInBytes(uint32_t Offset) const { return Offset * MinBlockSize; }

private:
	TAllocatorInitData InitData;

	const uint32_t MinBlockSize = 256;

	uint32_t MaxOrder;

	uint32_t TotalAllocSize = 0;

	std::vector<std::set<uint32_t>> FreeBlocks;

	std::vector<D3D12BuddyBlockData> DeferredDeletionQueue;

	ID3D12Device* D3DDevice;

	D3D12Resource* BackingResource = nullptr;

	ID3D12Heap* BackingHeap = nullptr;
};

