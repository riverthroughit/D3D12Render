#include "RenderTarget.h"

RenderTarget::RenderTarget(::D3D12RHI* InD3D12RHI, bool RenderDepth, UINT InWidth, UINT InHeight, DXGI_FORMAT InFormat, TVector4 InClearValue)
	: D3D12RHI(InD3D12RHI), bRenderDepth(RenderDepth), Width(InWidth), Height(InHeight), Format(InFormat), ClearValue(InClearValue)
{

}

RenderTarget::~RenderTarget()
{

}


RenderTarget2D::RenderTarget2D(::D3D12RHI* InD3D12RHI, bool RenderDepth, UINT InWidth, UINT InHeight, DXGI_FORMAT InFormat, TVector4 InClearValue)
	:RenderTarget(InD3D12RHI, RenderDepth, InWidth, InHeight, InFormat, InClearValue)
{
	CreateTexture();
}

void RenderTarget2D::CreateTexture()
{
	//Create D3DTexture
	TextureInfo TextureInfo;
	TextureInfo.Type = TextureType::TEXTURE_2D;
	TextureInfo.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	TextureInfo.Width = Width;
	TextureInfo.Height = Height;
	TextureInfo.Depth = 1;
	TextureInfo.MipCount = 1;
	TextureInfo.ArraySize = 1;
	TextureInfo.Format = Format;

	if (bRenderDepth)
	{
		TextureInfo.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		TextureInfo.SRVFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;

		D3DTexture = D3D12RHI->CreateTexture(TextureInfo, TexCreate_DSV | TexCreate_SRV);
	}
	else
	{
		D3DTexture = D3D12RHI->CreateTexture(TextureInfo, TexCreate_RTV | TexCreate_SRV, ClearValue);
	}
}

D3D12RenderTargetView* RenderTarget2D::GetRTV() const
{
	if (bRenderDepth)
	{
		return nullptr;
	}
	else
	{
		return D3DTexture->GetRTV();
	}
}

D3D12DepthStencilView* RenderTarget2D::GetDSV() const
{
	if (bRenderDepth)
	{
		return D3DTexture->GetDSV();
	}
	else
	{
		return nullptr;
	}
}

D3D12ShaderResourceView* RenderTarget2D::GetSRV() const
{
	return D3DTexture->GetSRV();
}

RenderTargetCube::RenderTargetCube(::D3D12RHI* InD3D12RHI, bool RenderDepth, UINT Size, DXGI_FORMAT InFormat, TVector4 InClearValue)
	:RenderTarget(InD3D12RHI, RenderDepth, Size, Size, InFormat, InClearValue)
{
	CreateTexture();
}

void RenderTargetCube::CreateTexture()
{
	//Create D3DTexture
	TextureInfo TextureInfo;
	TextureInfo.Type = TextureType::TEXTURE_CUBE;
	TextureInfo.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	TextureInfo.Width = Width;
	TextureInfo.Height = Height;
	TextureInfo.Depth = 1;
	TextureInfo.MipCount = 1;
	TextureInfo.ArraySize = 6;
	TextureInfo.Format = Format;

	if (bRenderDepth)
	{
		TextureInfo.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		TextureInfo.SRVFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;

		D3DTexture = D3D12RHI->CreateTexture(TextureInfo, TexCreate_CubeDSV | TexCreate_SRV);
	}
	else
	{
		D3DTexture = D3D12RHI->CreateTexture(TextureInfo, TexCreate_CubeRTV | TexCreate_SRV, ClearValue);
	}
}

D3D12RenderTargetView* RenderTargetCube::GetRTV(int Index) const
{
	if (bRenderDepth)
	{
		return nullptr;
	}
	else
	{
		return D3DTexture->GetRTV(Index);
	}
}

D3D12DepthStencilView* RenderTargetCube::GetDSV(int Index) const
{
	if (bRenderDepth)
	{
		return D3DTexture->GetDSV(Index);
	}
	else
	{
		return nullptr;
	}
}

D3D12ShaderResourceView* RenderTargetCube::GetSRV() const
{
	return D3DTexture->GetSRV();
}