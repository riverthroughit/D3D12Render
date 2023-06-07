#include "D3D12Render.h"

using namespace DirectX;






bool D3D12Render::initialize()
{
	mD3D12RHI = std::make_unique<D3D12RHI>();
	mD3D12RHI->Initialize(mhMainWnd, mClientWidth, mClientHeight);
	
	mD3DDevice = mD3D12RHI->GetDevice()->GetD3DDevice();
	mCommandList = mD3D12RHI->GetDevice()->GetCommandList();

	
	mGraphicsPSOManager = std::make_unique<GraphicsPSOManager>(mD3D12RHI.get(), &mInputLayoutManager);

	onResize(mClientWidth, mClientHeight);
	
	createRenderResource();

	beInitialized = true;

	return true;
}

void D3D12Render::onResize(int width,int height)
{
	mClientWidth = width;
	mClientHeight = height;

	mD3D12RHI->ResizeViewport(width, height);

	mCamera.updateProj(getAspectRatio());

	createGBuffers();

	createBackDepth();

	createColorTextures();

}

float D3D12Render::getAspectRatio() const
{
	return static_cast<float>(mClientWidth)/ mClientHeight;
}

void D3D12Render::createRenderResource()
{
	mD3D12RHI->ResetCommandList();

	createNullDescriptors();

	createMesh();

	createInputLayouts();

	createGlobalShaders();

	createGlobalPSO();

	mD3D12RHI->ExecuteCommandLists();
	mD3D12RHI->FlushCommandQueue();
}

void D3D12Render::createGBuffers()
{
	mGBufferBaseColor = std::make_unique<RenderTarget2D>(mD3D12RHI.get(), false, mClientWidth, mClientHeight, DXGI_FORMAT_R32G32B32A32_FLOAT);

	mGBufferNormal = std::make_unique<RenderTarget2D>(mD3D12RHI.get(), false, mClientWidth, mClientHeight, DXGI_FORMAT_R8G8B8A8_SNORM);

	mGBufferWorldPos = std::make_unique<RenderTarget2D>(mD3D12RHI.get(), false, mClientWidth, mClientHeight, DXGI_FORMAT_R32G32B32A32_FLOAT);

}

void D3D12Render::createBackDepth()
{
	//mBackDepth = std::make_unique<RenderTarget2D>(mD3D12RHI, true, mClientWidth, mClientHeight, DXGI_FORMAT_R24G8_TYPELESS);
}

void D3D12Render::createColorTextures()
{
	TextureInfo TextureInfo;
	TextureInfo.Type = TextureType::TEXTURE_2D;
	TextureInfo.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	TextureInfo.Width = mClientWidth;
	TextureInfo.Height = mClientHeight;
	TextureInfo.Depth = 1;
	TextureInfo.ArraySize = 1;
	TextureInfo.MipCount = 1;
	TextureInfo.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	TextureInfo.InitState = D3D12_RESOURCE_STATE_COMMON;

	mColorTexture = mD3D12RHI->CreateTexture(TextureInfo, TexCreate_SRV | TexCreate_RTV);

	mCacheColorTexture = mD3D12RHI->CreateTexture(TextureInfo, TexCreate_SRV);

	mPrevColorTexture = mD3D12RHI->CreateTexture(TextureInfo, TexCreate_SRV);
}

void D3D12Render::createNullDescriptors()
{
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc = {};
		SrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SrvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		SrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;

		mTexture2DNullDescriptor = std::make_unique<D3D12ShaderResourceView>(mD3D12RHI->GetDevice(), SrvDesc, nullptr);
	}

	{
		D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc = {};
		SrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SrvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		SrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;

		mTexture3DNullDescriptor = std::make_unique<D3D12ShaderResourceView>(mD3D12RHI->GetDevice(), SrvDesc, nullptr);
	}

	{
		D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc = {};
		SrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SrvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		SrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;

		mTextureCubeNullDescriptor = std::make_unique<D3D12ShaderResourceView>(mD3D12RHI->GetDevice(), SrvDesc, nullptr);
	}

	{
		D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc = {};
		SrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SrvDesc.Format = DXGI_FORMAT_UNKNOWN;
		SrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		SrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		SrvDesc.Buffer.StructureByteStride = 1;
		SrvDesc.Buffer.NumElements = 1;
		SrvDesc.Buffer.FirstElement = 0;

		mStructuredBufferNullDescriptor = std::make_unique<D3D12ShaderResourceView>(mD3D12RHI->GetDevice(), SrvDesc, nullptr);
	}
}

void D3D12Render::createMesh()
{
}

void D3D12Render::createInputLayouts()
{	
	//DefaultInputLayout
	std::vector<D3D12_INPUT_ELEMENT_DESC>  DefaultInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	mInputLayoutManager.AddInputLayout("DefaultInputLayout", DefaultInputLayout);
}

void D3D12Render::createGlobalShaders()
{
	{
		ShaderInfo ShaderInfo;
		ShaderInfo.ShaderName = "DeferredLighting";
		ShaderInfo.FileName = "DeferredLighting";
		//if (RenderSettings.bUseTBDR)
		//{
		//	ShaderInfo.ShaderDefines.SetDefine("USE_TBDR", "1");
		//}
		//if (RenderSettings.ShadowMapImpl == ShadowMapImpl::PCSS)
		//{
		//	ShaderInfo.ShaderDefines.SetDefine("USE_PCSS", "1");
		//}
		//else if (RenderSettings.ShadowMapImpl == ShadowMapImpl::VSM)
		//{
		//	ShaderInfo.ShaderDefines.SetDefine("USE_VSM", "1");
		//}
		//else if (RenderSettings.ShadowMapImpl == ShadowMapImpl::SDF)
		//{
		//	ShaderInfo.ShaderDefines.SetDefine("USE_SDF", "1");
		//}
		ShaderInfo.bCreateVS = true;
		ShaderInfo.bCreatePS = true;
		mDeferredLightingShader = std::make_unique<Shader>(ShaderInfo, mD3D12RHI.get());
	}
}

void D3D12Render::createGlobalPSO()
{
	// DeferredLighting
	{
		D3D12_DEPTH_STENCIL_DESC lightPassDSD;
		lightPassDSD.DepthEnable = false;
		lightPassDSD.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		lightPassDSD.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		lightPassDSD.StencilEnable = true;
		lightPassDSD.StencilReadMask = 0xff;
		lightPassDSD.StencilWriteMask = 0x0;
		lightPassDSD.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		lightPassDSD.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		lightPassDSD.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
		lightPassDSD.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
		// We are not rendering backfacing polygons, so these settings do not matter. 
		lightPassDSD.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		lightPassDSD.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		lightPassDSD.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
		lightPassDSD.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;

		auto blendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		blendState.AlphaToCoverageEnable = false;
		blendState.IndependentBlendEnable = false;

		blendState.RenderTarget[0].BlendEnable = true;
		blendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		blendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		blendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;

		auto rasterizer = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		//rasterizer.CullMode = D3D12_CULL_MODE_FRONT; // Front culling for point light
		rasterizer.CullMode = D3D12_CULL_MODE_NONE;
		rasterizer.DepthClipEnable = false;

		mDeferredLightingPSODescriptor.InputLayoutName = std::string("DefaultInputLayout");
		mDeferredLightingPSODescriptor.Shader = mDeferredLightingShader.get();
		mDeferredLightingPSODescriptor.BlendDesc = blendState;
		mDeferredLightingPSODescriptor.DepthStencilDesc = lightPassDSD;
		mDeferredLightingPSODescriptor.RasterizerDesc = rasterizer;
		mDeferredLightingPSODescriptor.RTVFormats[0] = DXGI_FORMAT_R32G32B32A32_FLOAT;
		mDeferredLightingPSODescriptor.NumRenderTargets = 1;
		mDeferredLightingPSODescriptor._4xMsaaState = false; //can't use msaa in deferred rendering.

		mGraphicsPSOManager->TryCreatePSO(mDeferredLightingPSODescriptor);
	}
}

void D3D12Render::updateData()
{

}

void D3D12Render::draw()
{

}


