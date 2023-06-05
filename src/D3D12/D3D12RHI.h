#pragma once

#include "D3D12Device.h"
#include "D3D12Viewport.h"
#include "D3D12Texture.h"
#include "D3D12Buffer.h"
//#include "Texture/TextureInfo.h"
#include "Math/MyMath.h"


class D3D12RHI
{
public:
	D3D12RHI();

	~D3D12RHI();

	void Initialize(HWND WindowHandle, int WindowWidth, int WindowHeight);

	void Destroy();

public:
	D3D12Device* GetDevice() { return Device.get(); }

	D3D12Viewport* GetViewport() { return Viewport.get(); }

	const D3D12ViewportInfo& GetViewportInfo();

	IDXGIFactory4* GetDxgiFactory();

public:
	//---------------------------RHI CommandList-----------------------------
	void FlushCommandQueue();

	void ExecuteCommandLists();

	void ResetCommandList();

	void ResetCommandAllocator();

	void Present();

	void ResizeViewport(int NewWidth, int NewHeight);

	void TransitionResource(D3D12Resource* Resource, D3D12_RESOURCE_STATES StateAfter);

	void CopyResource(D3D12Resource* DstResource, D3D12Resource* SrcResource);

	void CopyBufferRegion(D3D12Resource* DstResource, UINT64 DstOffset, D3D12Resource* SrcResource, UINT64 SrcOffset, UINT64 Size);

	void CopyTextureRegion(const D3D12_TEXTURE_COPY_LOCATION* Dst, UINT DstX, UINT DstY, UINT DstZ, const D3D12_TEXTURE_COPY_LOCATION* Src, const D3D12_BOX* SrcBox);

	D3D12ConstantBufferRef CreateConstantBuffer(const void* Contents, uint32_t Size);

	D3D12StructuredBufferRef CreateStructuredBuffer(const void* Contents, uint32_t ElementSize, uint32_t ElementCount);

	D3D12RWStructuredBufferRef CreateRWStructuredBuffer(uint32_t ElementSize, uint32_t ElementCount);

	D3D12VertexBufferRef CreateVertexBuffer(const void* Contents, uint32_t Size);

	D3D12IndexBufferRef CreateIndexBuffer(const void* Contents, uint32_t Size);

	D3D12ReadBackBufferRef CreateReadBackBuffer(uint32_t Size);

	D3D12TextureRef CreateTexture(const TTextureInfo& TextureInfo, uint32_t CreateFlags, TVector4 RTVClearValue = TVector4::Zero);

	// Use D3DResource to create texture, texture will manage this D3DResource
	D3D12TextureRef CreateTexture(Microsoft::WRL::ComPtr<ID3D12Resource> D3DResource, TTextureInfo& TextureInfo, uint32_t CreateFlags);

	void UploadTextureData(D3D12TextureRef Texture, const std::vector<D3D12_SUBRESOURCE_DATA>& InitData);

	void SetVertexBuffer(const D3D12VertexBufferRef& VertexBuffer, UINT Offset, UINT Stride, UINT Size);

	void SetIndexBuffer(const D3D12IndexBufferRef& IndexBuffer, UINT Offset, DXGI_FORMAT Format, UINT Size);

	void EndFrame();

	//-----------------------------------------------------------------------

private:
	void CreateDefaultBuffer(uint32_t Size, uint32_t Alignment, D3D12_RESOURCE_FLAGS Flags, D3D12ResourceLocation& ResourceLocation);

	void CreateAndInitDefaultBuffer(const void* Contents, uint32_t Size, uint32_t Alignment, D3D12ResourceLocation& ResourceLocation);

	D3D12TextureRef CreateTextureResource(const TTextureInfo& TextureInfo, uint32_t CreateFlags, TVector4 RTVClearValue);

	void CreateTextureViews(D3D12TextureRef TextureRef, const TTextureInfo& TextureInfo, uint32_t CreateFlags);

private:
	void LogAdapters();

	void LogAdapterOutputs(IDXGIAdapter* adapter);

	void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);

	UINT GetSupportMSAAQuality(DXGI_FORMAT BackBufferFormat);

private:
	std::unique_ptr<D3D12Device> Device = nullptr;

	std::unique_ptr<D3D12Viewport> Viewport = nullptr;

	D3D12ViewportInfo ViewportInfo;

	Microsoft::WRL::ComPtr<IDXGIFactory4> DxgiFactory = nullptr;
};

