#pragma once
#include <d3d12.h>
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <wrl.h>

#include <unordered_map>
#include<string>

struct SubmeshGeometry
{
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	INT BaseVertexLocation = 0;

	//��Χ��
	DirectX::BoundingBox Bounds;
};

struct MeshGeometry
{
	std::string Name;

	//cpu�ڴ��еļ������� ��blob�洢 ��Ҫʱ����ת������
	Microsoft::WRL::ComPtr<ID3DBlob> vertexBufferCPU = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> indexBufferCPU = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBufferGPU = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexBufferGPU = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBufferUploader = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexBufferUploader = nullptr;


	UINT vertexByteStride = 0;
	UINT vertexBufferByteSize = 0;
	DXGI_FORMAT indexFormat = DXGI_FORMAT_R16_UINT;
	UINT indexBufferByteSize = 0;

	//һ������������������п��ܴ洢���ģ�͵ļ�������
	std::unordered_map<std::string, SubmeshGeometry> DrawArgs;

	D3D12_VERTEX_BUFFER_VIEW VertexBufferView()const{
		D3D12_VERTEX_BUFFER_VIEW vbv;
		vbv.BufferLocation = vertexBufferGPU->GetGPUVirtualAddress();
		vbv.StrideInBytes = vertexByteStride;
		vbv.SizeInBytes = vertexBufferByteSize;

		return vbv;
	}

	D3D12_INDEX_BUFFER_VIEW IndexBufferView()const{
		D3D12_INDEX_BUFFER_VIEW ibv;
		ibv.BufferLocation = indexBufferGPU->GetGPUVirtualAddress();
		ibv.Format = indexFormat;
		ibv.SizeInBytes = indexBufferByteSize;

		return ibv;
	}

	//�ͷ��ϴ���
	void disposeUploaders(){
		vertexBufferUploader = nullptr;
		indexBufferUploader = nullptr;
	}
};

