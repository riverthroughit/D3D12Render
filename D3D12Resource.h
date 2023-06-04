#pragma once
#include"D3D12/tools/ToolForD12.h"

class D3D12BuddyAllocator;

//封装显存资源
class D3D12Resource
{
public:
	D3D12Resource(Microsoft::WRL::ComPtr<ID3D12Resource> InD3DResource, D3D12_RESOURCE_STATES InitState = D3D12_RESOURCE_STATE_COMMON);

	void Map();

public:
	Microsoft::WRL::ComPtr<ID3D12Resource> D3DResource = nullptr;

	D3D12_GPU_VIRTUAL_ADDRESS GPUVirtualAddress = 0;

	D3D12_RESOURCE_STATES CurrentState;

	// For upload buffer
	void* MappedBaseAddress = nullptr;
};

//
struct D3D12BuddyBlockData
{
	uint32_t Offset = 0;
	uint32_t Order = 0;
	uint32_t ActualUsedSize = 0;

	D3D12Resource* PlacedResource = nullptr;
};

//封装D3D12Resource 描述其分配方式(及其位于显存中的位置)
class D3D12ResourceLocation
{
public:
	enum class ResourceLocationType
	{
		Undefined,
		StandAlone,//CommittedResource创建显存并只用于一个D3D12Resource
		SubAllocation,//PlacedResource或ManualSubAllocation
	};


public:
	ResourceLocationType resourceLocationType = ResourceLocationType::Undefined;

	// SubAllocation 指向显存分配器
	D3D12BuddyAllocator* Allocator = nullptr;
	D3D12BuddyBlockData BlockData;

	
	D3D12Resource* UnderlyingResource = nullptr;

	union
	{
		uint64_t OffsetFromBaseOfResource;//PlacedResource
		uint64_t OffsetFromBaseOfHeap;//ManualSubAllocation
	};


	D3D12_GPU_VIRTUAL_ADDRESS GPUVirtualAddress = 0;

	// About mapping, for upload buffer
	void* MappedAddress = nullptr;


public:
	D3D12ResourceLocation() = default;

	~D3D12ResourceLocation();

	void ReleaseResource();

	void SetType(ResourceLocationType Type) { resourceLocationType = Type; }
};

