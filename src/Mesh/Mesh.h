#pragma once
#include<DirectXMath.h>
#include<vector>

struct Vertex
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT2 texcoord;
};

struct Mesh
{
	std::vector<Vertex> vertices;
	std::vector<std::uint32_t> indices;
};