#pragma once
#include <cassert>
#include<string>
#include<assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/mesh.h>
#include <assimp/scene.h>
#include <DirectXMath.h>
#include"Mesh.h"

class MeshLoader {

public:
	static void LoadModelsTo(const char* modelFilename,std::vector<Mesh>& meshes)
	{
		assert(modelFilename != nullptr);
		const std::string filePath(modelFilename);

		Assimp::Importer importer;
		const std::uint32_t flags{ aiProcessPreset_TargetRealtime_Fast | aiProcess_ConvertToLeftHanded };
		const aiScene* scene{ importer.ReadFile(filePath.c_str(), aiProcess_ConvertToLeftHanded |     // 转为左手系
		aiProcess_GenBoundingBoxes |        // 获取碰撞盒
		aiProcess_Triangulate |             // 将多边形拆分
		aiProcess_ImproveCacheLocality |    // 改善缓存局部性
		aiProcess_SortByPType) };
		
		assert(scene != nullptr);
		assert(scene->HasMeshes());

		for (std::uint32_t i = 0U; i < scene->mNumMeshes; ++i)
		{
			aiMesh* mesh = scene->mMeshes[i];
			assert(mesh != nullptr);

			Mesh tempMesh;

			{
				// Positions and Normals
				const std::uint32_t numVertices = mesh->mNumVertices;
				assert(numVertices > 0U);
				tempMesh.vertices.resize(numVertices);
				for (std::uint32_t i = 0U; i < numVertices; ++i)
				{
					tempMesh.vertices[i].position = DirectX::XMFLOAT3(reinterpret_cast<const float*>(&mesh->mVertices[i]));
					tempMesh.vertices[i].normal = DirectX::XMFLOAT3(reinterpret_cast<const float*>(&mesh->mNormals[i]));
				}

				// Indices
				const std::uint32_t numFaces = mesh->mNumFaces;
				assert(numFaces > 0U);
				for (std::uint32_t i = 0U; i < numFaces; ++i)
				{
					const aiFace* face = &mesh->mFaces[i];
					assert(face != nullptr);
					// We only allow triangles
					assert(face->mNumIndices == 3U);

					tempMesh.indices.push_back(face->mIndices[0U]);
					tempMesh.indices.push_back(face->mIndices[1U]);
					tempMesh.indices.push_back(face->mIndices[2U]);
				}

				// Texture Coordinates (if any)
				if (mesh->HasTextureCoords(0U))
				{
					assert(mesh->GetNumUVChannels() == 1U);
					const aiVector3D* aiTextureCoordinates{ mesh->mTextureCoords[0U] };
					assert(aiTextureCoordinates != nullptr);
					for (std::uint32_t i = 0U; i < numVertices; i++)
					{
						tempMesh.vertices[i].texcoord = DirectX::XMFLOAT2(reinterpret_cast<const float*>(&aiTextureCoordinates[i]));
					}
				}
			}

			meshes.push_back(std::move(tempMesh));
		}
	}
};