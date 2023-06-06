#include "D3D12Viewport.h"
#include "D3D12RHI.h"

D3D12Viewport::D3D12Viewport(::D3D12RHI* InD3D12RHI, const D3D12ViewportInfo& Info, int Width, int Height)
	:D3D12RHI(InD3D12RHI), ViewportInfo(Info), ViewportWidth(Width), ViewportHeight(Height)
{
	Initialize();
}

D3D12Viewport::~D3D12Viewport()
{

}

void D3D12Viewport::Initialize()
{
	CreateSwapChain();
}

void D3D12Viewport::CreateSwapChain()
{
	// Release the previous swapchain we will be recreating.
	SwapChain.Reset();

	DXGI_SWAP_CHAIN_DESC Desc;
	Desc.BufferDesc.Width = ViewportWidth;
	Desc.BufferDesc.Height = ViewportHeight;
	Desc.BufferDesc.RefreshRate.Numerator = 60;
	Desc.BufferDesc.RefreshRate.Denominator = 1;
	Desc.BufferDesc.Format = ViewportInfo.BackBufferFormat;
	Desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	Desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	Desc.SampleDesc.Count = ViewportInfo.bEnable4xMsaa ? 4 : 1;
	Desc.SampleDesc.Quality = ViewportInfo.bEnable4xMsaa ? (ViewportInfo.QualityOf4xMsaa - 1) : 0;
	Desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	Desc.BufferCount = SwapChainBufferCount;
	Desc.OutputWindow = ViewportInfo.WindowHandle;
	Desc.Windowed = true;
	Desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	Desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	// Note: Swap chain uses queue to perform flush.
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> CommandQueue = D3D12RHI->GetDevice()->GetCommandQueue();

	ThrowIfFailed(D3D12RHI->GetDxgiFactory()->CreateSwapChain(CommandQueue.Get(), &Desc, SwapChain.GetAddressOf()));
}

void D3D12Viewport::OnResize(int NewWidth, int NewHeight)
{
	ViewportWidth = NewWidth;
	ViewportHeight = NewHeight;

	// Flush before changing any resources.
	D3D12RHI->GetDevice()->GetCommandContext()->FlushCommandQueue();

	D3D12RHI->GetDevice()->GetCommandContext()->ResetCommandList();

	// Release the previous resources
	for (UINT i = 0; i < SwapChainBufferCount; i++)
	{
		RenderTargeTextures[i].reset();
	}
	DepthStencilTexture.reset();

	// Resize the swap chain.
	ThrowIfFailed(SwapChain->ResizeBuffers(SwapChainBufferCount, ViewportWidth, ViewportHeight,
		ViewportInfo.BackBufferFormat, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

	CurrBackBuffer = 0;

	// Create RenderTargeTextures
	for (UINT i = 0; i < SwapChainBufferCount; i++)
	{
		Microsoft::WRL::ComPtr<ID3D12Resource> SwapChainBuffer = nullptr;
		ThrowIfFailed(SwapChain->GetBuffer(i, IID_PPV_ARGS(&SwapChainBuffer)));

		D3D12_RESOURCE_DESC BackBufferDesc = SwapChainBuffer->GetDesc();

		TextureInfo TextureInfo;
		TextureInfo.RTVFormat = BackBufferDesc.Format;
		TextureInfo.InitState = D3D12_RESOURCE_STATE_PRESENT;
		RenderTargeTextures[i] = D3D12RHI->CreateTexture(SwapChainBuffer, TextureInfo, TexCreate_RTV);
	}

	// Create DepthStencilTexture
	TextureInfo TextureInfo;
	TextureInfo.Type = TextureType::TEXTURE_2D;
	TextureInfo.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	TextureInfo.Width = ViewportWidth;
	TextureInfo.Height = ViewportHeight;
	TextureInfo.Depth = 1;
	TextureInfo.MipCount = 1;
	TextureInfo.ArraySize = 1;
	TextureInfo.InitState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	TextureInfo.Format = DXGI_FORMAT_R24G8_TYPELESS;  // Create with a typeless format, support DSV and SRV(for SSAO)
	TextureInfo.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	TextureInfo.SRVFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;

	DepthStencilTexture = D3D12RHI->CreateTexture(TextureInfo, TexCreate_DSV | TexCreate_SRV);


	// Execute the resize commands.
	D3D12RHI->GetDevice()->GetCommandContext()->ExecuteCommandLists();

	// Wait until resize is complete.
	D3D12RHI->GetDevice()->GetCommandContext()->FlushCommandQueue();
}

void D3D12Viewport::GetD3DViewport(D3D12_VIEWPORT& OutD3DViewPort, D3D12_RECT& OutD3DRect)
{
	OutD3DViewPort.TopLeftX = 0;
	OutD3DViewPort.TopLeftY = 0;
	OutD3DViewPort.Width = static_cast<float>(ViewportWidth);
	OutD3DViewPort.Height = static_cast<float>(ViewportHeight);
	OutD3DViewPort.MinDepth = 0.0f;
	OutD3DViewPort.MaxDepth = 1.0f;

	OutD3DRect = { 0, 0, ViewportWidth, ViewportHeight };
}

void D3D12Viewport::Present()
{
	// swap the back and front buffers
	ThrowIfFailed(SwapChain->Present(0, 0));
	CurrBackBuffer = (CurrBackBuffer + 1) % SwapChainBufferCount;
}

D3D12Resource* D3D12Viewport::GetCurrentBackBuffer() const
{
	return RenderTargeTextures[CurrBackBuffer]->GetResource();
}

D3D12RenderTargetView* D3D12Viewport::GetCurrentBackBufferView() const
{
	return RenderTargeTextures[CurrBackBuffer]->GetRTV();
}

float* D3D12Viewport::GetCurrentBackBufferClearColor() const
{
	return RenderTargeTextures[CurrBackBuffer]->GetRTVClearValuePtr();
}

D3D12DepthStencilView* D3D12Viewport::GetDepthStencilView() const
{
	return DepthStencilTexture->GetDSV();
}

D3D12ShaderResourceView* D3D12Viewport::GetDepthShaderResourceView() const
{
	return DepthStencilTexture->GetSRV();
}

D3D12ViewportInfo D3D12Viewport::GetViewportInfo() const
{
	return ViewportInfo;
}