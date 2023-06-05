#pragma once

#include "D3D12CommandContext.h"
#include "D3D12MemoryAllocator.h"
#include "D3D12HeapSlotAllocator.h"

class D3D12RHI;

class D3D12Device
{
public:
	D3D12Device(D3D12RHI* InD3D12RHI);

	~D3D12Device();

public:
	ID3D12Device* GeD3DDevice() { return D3DDevice.Get(); }

	D3D12CommandContext* GetCommandContext() { return CommandContext.get(); }

	ID3D12CommandQueue* GetCommandQueue() { return CommandContext->GetCommandQueue(); }

	ID3D12GraphicsCommandList* GetCommandList() { return CommandContext->GetCommandList(); }

	D3D12UploadBufferAllocator* GetUploadBufferAllocator() { return UploadBufferAllocator.get(); }

	D3D12DefaultBufferAllocator* GetDefaultBufferAllocator() { return DefaultBufferAllocator.get(); }

	D3D3TextureResourceAllocator* GetTextureResourceAllocator() { return TextureResourceAllocator.get(); }

	D3D12HeapSlotAllocator* GetHeapSlotAllocator(D3D12_DESCRIPTOR_HEAP_TYPE HeapType);

private:
	void Initialize();

private:
	D3D12RHI* D3D12RHI = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Device> D3DDevice = nullptr;

	std::unique_ptr<D3D12CommandContext> CommandContext = nullptr;

private:
	std::unique_ptr<D3D12UploadBufferAllocator> UploadBufferAllocator = nullptr;

	std::unique_ptr<D3D12DefaultBufferAllocator> DefaultBufferAllocator = nullptr;

	std::unique_ptr<D3D3TextureResourceAllocator> TextureResourceAllocator = nullptr;

	std::unique_ptr<D3D12HeapSlotAllocator> RTVHeapSlotAllocator = nullptr;

	std::unique_ptr<D3D12HeapSlotAllocator> DSVHeapSlotAllocator = nullptr;

	std::unique_ptr<D3D12HeapSlotAllocator> SRVHeapSlotAllocator = nullptr;

};