#pragma once

#include "D3D12/D3D12Utils.h"
#include "InputLayout.h"
#include "Shader/Shader.h"
#include <unordered_map>

//-------------------------------------------------------------------------//
// GraphicsPSO
//-------------------------------------------------------------------------//

struct GraphicsPSODescriptor
{
	bool operator==(const GraphicsPSODescriptor& Other) const //TODO
	{
		return Other.InputLayoutName == InputLayoutName
			&& Other.Shader == Shader
			&& Other.PrimitiveTopologyType == PrimitiveTopologyType
			&& Other.RasterizerDesc.CullMode == RasterizerDesc.CullMode
			&& Other.DepthStencilDesc.DepthFunc == DepthStencilDesc.DepthFunc;
	}

public:
	std::string InputLayoutName;
	Shader* Shader = nullptr;
	DXGI_FORMAT RTVFormats[8] = { DXGI_FORMAT_R8G8B8A8_UNORM };
	bool _4xMsaaState = false;
	UINT _4xMsaaQuality = 0;
	DXGI_FORMAT DepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	D3D12_RASTERIZER_DESC RasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	D3D12_BLEND_DESC BlendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	D3D12_DEPTH_STENCIL_DESC DepthStencilDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	UINT NumRenderTargets = 1;
};

// declare hash<GraphicsPSODescriptor>
namespace std
{
	template <>
	struct hash<GraphicsPSODescriptor>
	{
		std::size_t operator()(const GraphicsPSODescriptor& Descriptor) const
		{
			using std::hash;
			using std::string;

			// Compute individual hash values for each item,
			// and combine them using XOR
			// and bit shifting:
			return (hash<string>()(Descriptor.InputLayoutName)
				^ (hash<void*>()(Descriptor.Shader) << 1));
		}
	};
}

class D3D12RHI;

class GraphicsPSOManager
{
public:
	GraphicsPSOManager(D3D12RHI* InD3D12RHI, InputLayoutManager* InInputLayoutManager);

	void TryCreatePSO(const GraphicsPSODescriptor& Descriptor);

	ID3D12PipelineState* GetPSO(const GraphicsPSODescriptor& Descriptor) const;

private:
	void CreatePSO(const GraphicsPSODescriptor& Descriptor);

private:
	D3D12RHI* D3D12RHI = nullptr;

	InputLayoutManager* inputLayoutManager = nullptr;

	std::unordered_map<GraphicsPSODescriptor, Microsoft::WRL::ComPtr<ID3D12PipelineState>> PSOMap;
};

//-------------------------------------------------------------------------//
// ComputePSO
//-------------------------------------------------------------------------//

struct ComputePSODescriptor
{
	bool operator==(const ComputePSODescriptor& Other) const 
	{
		return Other.Shader == Shader
			&& Other.Flags == Flags;
	}

public:
	Shader* Shader = nullptr;
	D3D12_PIPELINE_STATE_FLAGS Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
};

// declare hash<ComputePSODescriptor>
namespace std
{
	template <>
	struct hash<ComputePSODescriptor>
	{
		std::size_t operator()(const ComputePSODescriptor& Descriptor) const
		{
			using std::hash;
			using std::string;

			// Compute individual hash values for each item,
			// and combine them using XOR
			// and bit shifting:
			return (hash<void*>()(Descriptor.Shader)
				^ (hash<int>()(Descriptor.Flags) << 1));
		}
	};
}

class ComputePSOManager
{
public:
	ComputePSOManager(D3D12RHI* InD3D12RHI);

	void TryCreatePSO(const ComputePSODescriptor& Descriptor);

	ID3D12PipelineState* GetPSO(const ComputePSODescriptor& Descriptor) const;

private:
	void CreatePSO(const ComputePSODescriptor& Descriptor);

private:
	class D3D12RHI* D3D12RHI = nullptr;

	std::unordered_map<ComputePSODescriptor, Microsoft::WRL::ComPtr<ID3D12PipelineState>> PSOMap;
};