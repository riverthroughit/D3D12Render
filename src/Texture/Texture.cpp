#include "Texture.h"
#include "TextureLoader/DDSTextureLoader.h"
#include "TextureLoader/WICTextureLoader.h"
#include "TextureLoader/HDRTextureLoader.h"
#include "Utils/FormatConvert.h"

void Texture::LoadTextureResourceFromFlie(D3D12RHI* D3D12RHI)
{
	std::wstring ext = GetExtension(FilePath);
	if (ext == L"dds")
	{
		LoadDDSTexture(D3D12RHI->GetDevice());
	}
	else if (ext == L"png" || ext == L"jpg")
	{
		LoadWICTexture(D3D12RHI->GetDevice());
	}
	else if (ext == L"hdr")
	{
		LoadHDRTexture(D3D12RHI->GetDevice());
	}
}

std::wstring Texture::GetExtension(std::wstring path)
{
	if ((path.rfind('.') != std::wstring::npos) && (path.rfind('.') != (path.length() - 1)))
		return path.substr(path.rfind('.') + 1);
	else
		return L"";
}

void Texture::LoadDDSTexture(D3D12Device* Device)
{
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile(FilePath.c_str(), TextureResource.TextureInfo, 
		TextureResource.InitData, TextureResource.TextureData, bSRGB));
}

void Texture::LoadWICTexture(D3D12Device* Device)
{
	D3D12_SUBRESOURCE_DATA InitData;

	DirectX::WIC_LOADER_FLAGS LoadFlags;
	if (bSRGB)
	{
		LoadFlags = DirectX::WIC_LOADER_FORCE_SRGB;
	}
	else
	{
		LoadFlags = DirectX::WIC_LOADER_IGNORE_SRGB;
	}

	ThrowIfFailed(DirectX::CreateWICTextureFromFile(FilePath.c_str(), 0u, D3D12_RESOURCE_FLAG_NONE, LoadFlags,
		TextureResource.TextureInfo, InitData, TextureResource.TextureData));

	TextureResource.InitData.push_back(InitData);
}

void Texture::LoadHDRTexture(D3D12Device* Device)
{
	D3D12_SUBRESOURCE_DATA InitData;

	CreateHDRTextureFromFile(FormatConvert::WStrToStr(FilePath), TextureResource.TextureInfo, InitData, TextureResource.TextureData);

	TextureResource.InitData.push_back(InitData);
}

void Texture::SeTextureResourceDirectly(const TextureInfo& InTextureInfo, const std::vector<uint8_t>& InTextureData, const D3D12_SUBRESOURCE_DATA& InInitData)
{
	TextureResource.TextureInfo = InTextureInfo;
	TextureResource.TextureData = InTextureData;

	D3D12_SUBRESOURCE_DATA InitData;
	InitData.pData = TextureResource.TextureData.data();
	InitData.RowPitch = InInitData.RowPitch;
	InitData.SlicePitch = InInitData.SlicePitch;

	TextureResource.InitData.push_back(InitData);
}

void Texture::CreateTexture(D3D12RHI* D3D12RHI)
{
	auto CommandList = D3D12RHI->GetDevice()->GetCommandList();

	//Create D3DTexture
	auto& TextureInfo = TextureResource.TextureInfo;
	TextureInfo.Type = Type;
	D3DTexture = D3D12RHI->CreateTexture(TextureInfo, TexCreate_SRV);

	//Upload InitData
	D3D12RHI->UploadTextureData(D3DTexture, TextureResource.InitData);
}


Texture2D::Texture2D(const std::string& InName, bool InbSRGB, std::wstring InFilePath)
	:Texture(InName, TextureType::TEXTURE_2D, InbSRGB, InFilePath)
{

}

Texture2D::~Texture2D()
{

}

TextureCube::TextureCube(const std::string& InName, bool InbSRGB, std::wstring InFilePath)
	:Texture(InName, TextureType::TEXTURE_CUBE, InbSRGB, InFilePath)
{

}

TextureCube::~TextureCube()
{

}

Texture3D::Texture3D(const std::string& InName, bool InbSRGB, std::wstring InFilePath)
	:Texture(InName, TextureType::TEXTURE_3D, InbSRGB, InFilePath)
{

}

Texture3D::~Texture3D()
{

}