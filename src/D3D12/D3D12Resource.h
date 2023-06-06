#pragma once

#include "D3D12Utils.h"

class D3D12BuddyAllocator;


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

struct D3D12BuddyBlockData
{
	uint32_t Offset = 0;
	uint32_t Order = 0;
	uint32_t ActualUsedSize = 0;

	D3D12Resource* PlacedResource = nullptr;
};

class D3D12ResourceLocation
{
public:
	enum class EResourceLocationType
	{
		Undefined,
		StandAlone,
		SubAllocation,
	};

public:
	D3D12ResourceLocation();

	~D3D12ResourceLocation();

	void ReleaseResource();

	void SetType(EResourceLocationType Type) { ResourceLocationType = Type; }

public:
	EResourceLocationType ResourceLocationType = EResourceLocationType::Undefined;

    // SubAllocation 
	D3D12BuddyAllocator* Allocator = nullptr;

	D3D12BuddyBlockData BlockData;

    // StandAlone resource 
	D3D12Resource* UnderlyingResource = nullptr;

	union 
	{
		uint64_t OffsetFromBaseOfResource;
		uint64_t OffsetFromBaseOfHeap;
	};
	

	D3D12_GPU_VIRTUAL_ADDRESS GPUVirtualAddress = 0;

	// About mapping, for upload buffer
	void* MappedAddress = nullptr;
};

template<typename T>
class D3D12ScopeMap
{
public:
	D3D12ScopeMap(D3D12Resource* Resource)
	{
		D3DResource = Resource->D3DResource.Get();
		D3DResource->Map(0, nullptr, reinterpret_cast<void**>(&MappedData));
	}

	~D3D12ScopeMap()
	{
		D3DResource->Unmap(0, nullptr);
	}

	T* GetMappedData() { return MappedData; }

private:
	ID3D12Resource* D3DResource = nullptr;

	T* MappedData = nullptr;
};
