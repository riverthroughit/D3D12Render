#pragma once
#include "D3D12Resource.h"
#include<set>

//也许可放入cpp文件
#define DEFAULT_POOL_SIZE (512 * 1024 * 512)

#define DEFAULT_RESOURCE_ALIGNMENT 4
#define UPLOAD_RESOURCE_ALIGNMENT 256

//基于伙伴系统的显存分配
class D3D12BuddyAllocator
{
public:
	enum class AllocationStrategy
	{
		PlacedResource,//将一个heap划分为不同部分
		ManualSubAllocation//一次CommittedResource的资源划分为不同部分
	};

	struct AllocatorInitData
	{
		AllocationStrategy allocationStrategy;

		D3D12_HEAP_TYPE HeapType;

		D3D12_HEAP_FLAGS HeapFlags = D3D12_HEAP_FLAG_NONE;  // Only for PlacedResource

		D3D12_RESOURCE_FLAGS ResourceFlags = D3D12_RESOURCE_FLAG_NONE;  // Only for ManualSubAllocation
	};


	private:
	AllocatorInitData InitData;

	const uint32_t MinBlockSize = 256;

	uint32_t MaxOrder;//最大阶数

	uint32_t TotalAllocSize = 0;

	std::vector<std::set<uint32_t>> FreeBlocks;

	std::vector<D3D12BuddyBlockData> DeferredDeletionQueue;

	ID3D12Device* D3DDevice;

	D3D12Resource* BackingResource = nullptr;//ManualSubAllocation

	ID3D12Heap* BackingHeap = nullptr;//PlacedResource

public:
	D3D12BuddyAllocator(ID3D12Device* InDevice, const AllocatorInitData& InInitData);

	~D3D12BuddyAllocator();

	bool AllocResource(uint32_t Size, uint32_t Alignment, D3D12ResourceLocation& ResourceLocation);

	void Deallocate(D3D12ResourceLocation& ResourceLocation);

	void CleanUpAllocations();

	ID3D12Heap* GetBackingHeap() { return BackingHeap; }

	AllocationStrategy GetAllocationStrategy() { return InitData.allocationStrategy; }

private:
	void Initialize();

	uint32_t AllocateBlock(uint32_t Order);

	//没懂 
	uint32_t GetSizeToAllocate(uint32_t Size, uint32_t Alignment);

	bool CanAllocate(uint32_t SizeToAllocate);

	//根据显存Size大小计算显存块的数量
	uint32_t SizeToUnitSize(uint32_t Size) const
	{
		return (Size + (MinBlockSize - 1)) / MinBlockSize;
	}

	//根据显存块的数量计算阶数
	uint32_t UnitSizeToOrder(uint32_t Size) const
	{
		unsigned long Result;
		_BitScanReverse(&Result, Size + Size - 1); // ceil(log2(size))
		return Result;
	}

	//根据阶数计算显存块的数量
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


};


class D3D12MultiBuddyAllocator
{
public:
	D3D12MultiBuddyAllocator(ID3D12Device* InDevice, const D3D12BuddyAllocator::AllocatorInitData& InInitData);

	~D3D12MultiBuddyAllocator();

	bool AllocResource(uint32_t Size, uint32_t Alignment, D3D12ResourceLocation& ResourceLocation);

	void CleanUpAllocations();

private:
	std::vector<std::shared_ptr<D3D12BuddyAllocator>> Allocators;

	ID3D12Device* Device;

	D3D12BuddyAllocator::AllocatorInitData InitData;
};

class D3D12UploadBufferAllocator
{
public:
	D3D12UploadBufferAllocator(ID3D12Device* InDevice);

	void* AllocUploadResource(uint32_t Size, uint32_t Alignment, D3D12ResourceLocation& ResourceLocation);

	void CleanUpAllocations();

private:
	std::unique_ptr<D3D12MultiBuddyAllocator> Allocator = nullptr;

	ID3D12Device* D3DDevice = nullptr;
};

class D3D12DefaultBufferAllocator
{
public:
	D3D12DefaultBufferAllocator(ID3D12Device* InDevice);

	void AllocDefaultResource(const D3D12_RESOURCE_DESC& ResourceDesc, uint32_t Alignment, D3D12ResourceLocation& ResourceLocation);

	void CleanUpAllocations();

private:
	std::unique_ptr<D3D12MultiBuddyAllocator> Allocator = nullptr;

	std::unique_ptr<D3D12MultiBuddyAllocator> UavAllocator = nullptr;

	ID3D12Device* D3DDevice = nullptr;
};

class D3D3TextureResourceAllocator
{
public:
	D3D3TextureResourceAllocator(ID3D12Device* InDevice);

	void AllocTextureResource(const D3D12_RESOURCE_STATES& ResourceState, const D3D12_RESOURCE_DESC& ResourceDesc, D3D12ResourceLocation& ResourceLocation);

	void CleanUpAllocations();

private:
	std::unique_ptr<D3D12MultiBuddyAllocator> Allocator = nullptr;

	ID3D12Device* D3DDevice = nullptr;
};
