#include "D3D12Resource.h"

D3D12Resource::D3D12Resource(Microsoft::WRL::ComPtr<ID3D12Resource> InD3DResource, D3D12_RESOURCE_STATES InitState)
	:D3DResource(InD3DResource), CurrentState(InitState)
{
	if (D3DResource->GetDesc().Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
	{
		GPUVirtualAddress = D3DResource->GetGPUVirtualAddress();
	}
	else
	{
		// GetGPUVirtualAddress() returns NULL for non-buffer resources.
	}
}

void D3D12Resource::Map()
{
	ThrowIfFailed(D3DResource->Map(0, nullptr, &MappedBaseAddress));
}