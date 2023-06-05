#pragma once

#include <string>
#include "Texture/TextureInfo.h"
#include "D3D12/D3D12Texture.h"
#include "D3D12/D3D12RHI.h"

struct TextureResource
{
	TextureInfo TextureInfo;

	std::vector<uint8_t> TextureData;

	std::vector<D3D12_SUBRESOURCE_DATA> InitData;
};

class Texture
{
public:
	Texture(const std::string& InName, TextureType InType, bool InbSRGB, std::wstring InFilePath)
		:Name(InName), Type(InType), bSRGB(InbSRGB), FilePath(InFilePath)
	{}

	virtual ~Texture()
	{}

	Texture(const Texture& Other) = delete;

	Texture& operator=(const Texture& Other) = delete;

public:
	void LoadTextureResourceFromFlie(D3D12RHI* D3D12RHI);

	void SeTextureResourceDirectly(const TextureInfo& InTextureInfo, const std::vector<uint8_t>& InTextureData, 
		const D3D12_SUBRESOURCE_DATA& InInitData);

	void CreateTexture(D3D12RHI* D3D12RHI);

	D3D12TextureRef GeD3DTexture() { return D3DTexture; }

private:
	static std::wstring GetExtension(std::wstring path);

	void LoadDDSTexture(D3D12Device* Device);

	void LoadWICTexture(D3D12Device* Device);

	void LoadHDRTexture(D3D12Device* Device);

public:
	std::string Name;

	TextureType Type;

	std::wstring FilePath;

	bool bSRGB = true;

	TextureResource TextureResource;

	D3D12TextureRef D3DTexture = nullptr;
};

class Texture2D : public Texture
{
public:
	Texture2D(const std::string& InName, bool InbSRGB, std::wstring InFilePath);

	~Texture2D();
};

class TextureCube : public Texture
{
public:
	TextureCube(const std::string& InName, bool InbSRGB, std::wstring InFilePath);

	~TextureCube();
};

class Texture3D : public Texture
{
public:
	Texture3D(const std::string& InName, bool InbSRGB, std::wstring InFilePath);

	~Texture3D();
};
