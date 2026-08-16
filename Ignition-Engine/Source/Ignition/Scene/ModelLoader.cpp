#include "Ignition/Scene/ModelLoader.h"

#include "Ignition/Core/Log.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <SDL3/SDL_filesystem.h>

#include <cstring>

namespace Ignition
{
	namespace
	{
		std::string GetParentDirectory(const std::string& filepath)
		{
			const size_t separator = filepath.find_last_of("/\\");

			return separator == std::string::npos ? std::string() : filepath.substr(0, separator + 1);
		}

		int FindEmbeddedTextureIndex(const aiScene& importedScene, const aiTexture* embedded)
		{
			for (uint32_t index = 0; index < importedScene.mNumTextures; ++index)
			{
				if (importedScene.mTextures[index] == embedded)
				{
					return static_cast<int>(index);
				}
			}

			return -1;
		}

		void ReadMaterial(const aiScene& importedScene, const aiMaterial& importedMaterial, const std::string& modelDirectory, ModelSubmesh& outSubmesh)
		{
			aiString path;

			if (importedMaterial.GetTexture(aiTextureType_BASE_COLOR, 0, &path) == aiReturn_SUCCESS || importedMaterial.GetTexture(aiTextureType_DIFFUSE, 0, &path) == aiReturn_SUCCESS)
			{
				if (const aiTexture* embedded = importedScene.GetEmbeddedTexture(path.C_Str()))
				{
					outSubmesh.EmbeddedAlbedoIndex = FindEmbeddedTextureIndex(importedScene, embedded);
				}
				else
				{
					outSubmesh.AlbedoPath = modelDirectory + path.C_Str();
				}
			}

			aiColor4D baseColor{ 1.0f, 1.0f, 1.0f, 1.0f };

			if (importedMaterial.Get(AI_MATKEY_BASE_COLOR, baseColor) == aiReturn_SUCCESS || importedMaterial.Get(AI_MATKEY_COLOR_DIFFUSE, baseColor) == aiReturn_SUCCESS)
			{
				outSubmesh.Tint = { baseColor.r, baseColor.g, baseColor.b, baseColor.a };
			}

			int twoSided = 0;

			if (importedMaterial.Get(AI_MATKEY_TWOSIDED, twoSided) == aiReturn_SUCCESS)
			{
				outSubmesh.TwoSided = twoSided != 0;
			}
		}

		void ReadEmbeddedTextures(const aiScene& importedScene, ModelData& outModel)
		{
			// Indices must line up with aiScene::mTextures so a "#tex:N" reference stays stable
			outModel.EmbeddedTextures.resize(importedScene.mNumTextures);

			for (uint32_t index = 0; index < importedScene.mNumTextures; ++index)
			{
				const aiTexture& embedded = *importedScene.mTextures[index];

				if (embedded.mHeight != 0)
				{
					IG_CORE_WARN("Model loader: uncompressed embedded texture {} is not supported yet", index);

					continue;
				}

				const auto* bytes = reinterpret_cast<const uint8_t*>(embedded.pcData);
				outModel.EmbeddedTextures[index].Data.assign(bytes, bytes + embedded.mWidth);
			}
		}
	}

	bool LoadModelFile(const std::string& filepath, ModelData& outModel)
	{
		const char* basePath = SDL_GetBasePath();
		const std::string fullPath = std::string(basePath ? basePath : "") + filepath;

		Assimp::Importer importer;

		const aiScene* importedScene = importer.ReadFile(fullPath, aiProcess_Triangulate | aiProcess_PreTransformVertices | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices);

		if (!importedScene || !importedScene->HasMeshes())
		{
			IG_CORE_ERROR("Model load failed for '{}': {}", filepath, importer.GetErrorString());

			return false;
		}

		ReadEmbeddedTextures(*importedScene, outModel);

		const std::string modelDirectory = GetParentDirectory(filepath);

		outModel.Submeshes.reserve(importedScene->mNumMeshes);

		for (uint32_t meshIndex = 0; meshIndex < importedScene->mNumMeshes; ++meshIndex)
		{
			const aiMesh& importedMesh = *importedScene->mMeshes[meshIndex];

			if (!importedMesh.HasPositions() || !importedMesh.HasFaces())
			{
				continue;
			}

			ModelSubmesh submesh;
			submesh.Name = importedMesh.mName.C_Str();

			if (submesh.Name.empty())
			{
				submesh.Name = "Mesh " + std::to_string(meshIndex);
			}

			submesh.Vertices.reserve(importedMesh.mNumVertices);

			for (uint32_t vertexIndex = 0; vertexIndex < importedMesh.mNumVertices; ++vertexIndex)
			{
				Vertex vertex{};
				vertex.Position = { importedMesh.mVertices[vertexIndex].x, importedMesh.mVertices[vertexIndex].y, importedMesh.mVertices[vertexIndex].z };
				vertex.Normal = importedMesh.HasNormals() ? glm::vec3{ importedMesh.mNormals[vertexIndex].x, importedMesh.mNormals[vertexIndex].y, importedMesh.mNormals[vertexIndex].z } : glm::vec3{ 0.0f, 0.0f, 1.0f };
				vertex.Color = importedMesh.HasVertexColors(0) ? glm::vec3{ importedMesh.mColors[0][vertexIndex].r, importedMesh.mColors[0][vertexIndex].g, importedMesh.mColors[0][vertexIndex].b } : glm::vec3{ 1.0f };
				vertex.UV = importedMesh.HasTextureCoords(0) ? glm::vec2{ importedMesh.mTextureCoords[0][vertexIndex].x, importedMesh.mTextureCoords[0][vertexIndex].y } : glm::vec2{ 0.0f };

				submesh.Vertices.push_back(vertex);
			}

			submesh.Indices.reserve(static_cast<size_t>(importedMesh.mNumFaces) * 3);

			for (uint32_t faceIndex = 0; faceIndex < importedMesh.mNumFaces; ++faceIndex)
			{
				const aiFace& face = importedMesh.mFaces[faceIndex];

				if (face.mNumIndices != 3)
				{
					continue;
				}

				submesh.Indices.push_back(face.mIndices[0]);
				submesh.Indices.push_back(face.mIndices[1]);
				submesh.Indices.push_back(face.mIndices[2]);
			}

			if (submesh.Indices.empty())
			{
				continue;
			}

			if (importedMesh.mMaterialIndex < importedScene->mNumMaterials)
			{
				ReadMaterial(*importedScene, *importedScene->mMaterials[importedMesh.mMaterialIndex], modelDirectory, submesh);
			}

			outModel.Submeshes.push_back(std::move(submesh));
		}

		return !outModel.Submeshes.empty();
	}
}