#pragma once

#include "D3D12/D3D12Texture.h"
#include "D3D12/D3D12RHI.h"

class RenderTarget
{
protected:
public:
	RenderTarget(D3D12RHI* InD3D12RHI, bool RenderDepth, UINT InWidth, UINT InHeight, DXGI_FORMAT InFormat, TVector4 InClearValue = TVector4::Zero);

	virtual ~RenderTarget();

public:
	D3D12TextureRef GetTexture() const { return D3DTexture; }

	D3D12Resource* GetResource() const { return D3DTexture->GetResource(); }

	DXGI_FORMAT GetFormat() const { return Format; }

	TVector4 GetClearColor()  { return ClearValue; }

	float* GetClearColorPtr() { return (float*)&ClearValue; }

protected:
	bool bRenderDepth = false;

	D3D12RHI* D3D12RHI = nullptr;

	D3D12TextureRef D3DTexture = nullptr;

	UINT Width;

	UINT Height;

	DXGI_FORMAT Format;

	TVector4 ClearValue;
};

class RenderTarget2D : public RenderTarget
{
public:
	RenderTarget2D(::D3D12RHI* InD3D12RHI, bool RenderDepth, UINT InWidth, UINT InHeight, DXGI_FORMAT InFormat, TVector4 InClearValue = TVector4::Zero);

	D3D12RenderTargetView* GetRTV() const;

	D3D12DepthStencilView* GetDSV() const;

	D3D12ShaderResourceView* GetSRV() const;

private:
	void CreateTexture();
};

class RenderTargetCube : public RenderTarget
{
public:
	RenderTargetCube(::D3D12RHI* InD3D12RHI, bool RenderDepth, UINT Size, DXGI_FORMAT InFormat, TVector4 InClearValue = TVector4::Zero);

	D3D12RenderTargetView* GetRTV(int Index) const;

	D3D12DepthStencilView* GetDSV(int Index) const;

	D3D12ShaderResourceView* GetSRV() const;

private:
	void CreateTexture();
};