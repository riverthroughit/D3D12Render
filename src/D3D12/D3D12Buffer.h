#pragma once

#include "D3D12Resource.h"
#include "D3D12View.h"

class D3D12Buffer
{
public:
	D3D12Buffer() {}

	virtual ~D3D12Buffer() {}

	D3D12Resource* GetResource() { return ResourceLocation.UnderlyingResource; }

public:
	D3D12ResourceLocation ResourceLocation;
};

class D3D12ConstantBuffer : public D3D12Buffer
{

};
typedef std::shared_ptr<D3D12ConstantBuffer> D3D12ConstantBufferRef;


class D3D12StructuredBuffer : public D3D12Buffer
{
public:
	D3D12ShaderResourceView* GetSRV()
	{
		return SRV.get();
	}

	void SetSRV(std::unique_ptr<D3D12ShaderResourceView>& InSRV)
	{
		SRV = std::move(InSRV);
	}

private:
	std::unique_ptr<D3D12ShaderResourceView> SRV = nullptr;
};
typedef std::shared_ptr<D3D12StructuredBuffer> D3D12StructuredBufferRef;


class D3D12RWStructuredBuffer : public D3D12Buffer
{
public:
	D3D12ShaderResourceView* GetSRV()
	{
		return SRV.get();
	}

	void SetSRV(std::unique_ptr<D3D12ShaderResourceView>& InSRV)
	{
		SRV = std::move(InSRV);
	}

	D3D12UnorderedAccessView* GetUAV()
	{
		return UAV.get();
	}

	void SetUAV(std::unique_ptr<D3D12UnorderedAccessView>& InUAV)
	{
		UAV = std::move(InUAV);
	}

private:
	std::unique_ptr<D3D12ShaderResourceView> SRV = nullptr;

	std::unique_ptr<D3D12UnorderedAccessView> UAV = nullptr;
};
typedef std::shared_ptr<D3D12RWStructuredBuffer> D3D12RWStructuredBufferRef;


class D3D12VertexBuffer : public D3D12Buffer
{

};
typedef std::shared_ptr<D3D12VertexBuffer> D3D12VertexBufferRef;


class D3D12IndexBuffer : public D3D12Buffer
{

};
typedef std::shared_ptr<D3D12IndexBuffer> D3D12IndexBufferRef;


class D3D12ReadBackBuffer : public D3D12Buffer
{

};
typedef std::shared_ptr<D3D12ReadBackBuffer> D3D12ReadBackBufferRef;