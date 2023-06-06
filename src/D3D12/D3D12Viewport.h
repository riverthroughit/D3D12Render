#pragma once

#include "D3D12Utils.h"
#include "D3D12Texture.h"

class D3D12RHI;

struct D3D12ViewportInfo
{
	HWND WindowHandle;

	DXGI_FORMAT BackBufferFormat;
	DXGI_FORMAT DepthStencilFormat;

	bool bEnable4xMsaa = false;    // 4X MSAA enabled
	UINT QualityOf4xMsaa = 0;      // quality level of 4X MSAA
};

class D3D12Viewport
{
public:
	D3D12Viewport(D3D12RHI* InD3D12RHI, const D3D12ViewportInfo& Info, int Width, int Height);

	~D3D12Viewport();

public:
	void OnResize(int NewWidth, int NewHeight);

	void GetD3DViewport(D3D12_VIEWPORT& OutD3DViewPort, D3D12_RECT& OutD3DRect);

	void Present();

	D3D12Resource* GetCurrentBackBuffer() const;

	D3D12RenderTargetView* GetCurrentBackBufferView() const;

	float* GetCurrentBackBufferClearColor() const;

	D3D12DepthStencilView* GetDepthStencilView() const;

	D3D12ShaderResourceView* GetDepthShaderResourceView() const;

	D3D12ViewportInfo GetViewportInfo() const;

private:
	void Initialize();

	void CreateSwapChain();

private:
	D3D12RHI* D3D12RHI = nullptr;

	D3D12ViewportInfo ViewportInfo;
	int ViewportWidth = 0;
	int ViewportHeight = 0;

	static const int SwapChainBufferCount = 2;	
	Microsoft::WRL::ComPtr<IDXGISwapChain> SwapChain = nullptr;
	int CurrBackBuffer = 0;
	
	D3D12TextureRef RenderTargeTextures[SwapChainBufferCount];

	D3D12TextureRef DepthStencilTexture = nullptr;
};