#include "Ignition/Scene/ModelImporter.h"

#include "Ignition/Scene/Scene.h"
#include "Ignition/Scene/Components.h"
#include "Ignition/Renderer/Renderer.h"
#include "Ignition/Renderer/Mesh.h"
#include "Ignition/Renderer/Texture.h"
#include "Ignition/Renderer/Material.h"
#include "Ignition/Renderer/Vertex.h"
#include "Ignition/Core/Log.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <SDL3/SDL_filesystem.h>

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Ignition
{
	namespace
	{
		std::string GetParentDirectory(const std::string& filepath)
		{
			const size_t separator = filepath.find_last_of("/\\");

			return separator == std::string::npos ? std::string() : filepath.substr(0, separator + 1);
		}

		std::shared_ptr<Texture> LoadMaterialTexture(Renderer& renderer, const aiScene& importedScene, const aiMaterial& importedMaterial, const std::string& modelDirectory, std::unordered_map<std::string, std::shared_ptr<Texture>>& textureCache)
		{
			aiString path;

			if (importedMaterial.GetTexture(aiTextureType_BASE_COLOR, 0, &path) != aiReturn_SUCCESS && importedMaterial.GetTexture(aiTextureType_DIFFUSE, 0, &path) != aiReturn_SUCCESS)
			{
				return nullptr;
			}

			const std::string key = path.C_Str();

			if (const auto cached = textureCache.find(key); cached != textureCache.end())
			{
				return cached->second;
			}

			std::shared_ptr<Texture> texture;

			if (const aiTexture* embedded = importedScene.GetEmbeddedTexture(path.C_Str()))
			{
				if (embedded->mHeight == 0)
				{
					texture = renderer.CreateTextureFromMemory(embedded->pcData, static_cast<size_t>(embedded->mWidth));
				}
				else
				{
					IG_CORE_WARN("Model importer: uncompressed embedded texture '{}' is not supported yet", key);
				}
			}
			else
			{
				texture = renderer.CreateTexture(modelDirectory + key);
			}

			textureCache.emplace(key, texture);

			return texture;
		}
	}

	std::vector<Entity> ModelImporter::Import(Scene& scene, Renderer& renderer, const std::string& filepath)
	{
		IG_CORE_INFO("------- IMPORTING MODEL '{}' -------", filepath);

		const char* basePath = SDL_GetBasePath();
		const std::string fullPath = std::string(basePath ? basePath : "") + filepath;

		Assimp::Importer importer;

		// No aiProcess_FlipUVs: the stb loader's vertical image flip is the engine's
		// single UV flip, and assimp's output convention already matches it.
		const aiScene* importedScene = importer.ReadFile(fullPath, aiProcess_Triangulate | aiProcess_PreTransformVertices | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices);

		if (!importedScene || !importedScene->HasMeshes())
		{
			IG_CORE_ERROR("Model import failed for '{}': {}", filepath, importer.GetErrorString());

			return {};
		}

		const std::string modelDirectory = GetParentDirectory(filepath);

		std::unordered_map<std::string, std::shared_ptr<Texture>> textureCache;

		std::vector<Entity> entities;
		entities.reserve(importedScene->mNumMeshes);

		for (uint32_t meshIndex = 0; meshIndex < importedScene->mNumMeshes; ++meshIndex)
		{
			const aiMesh& importedMesh = *importedScene->mMeshes[meshIndex];

			if (!importedMesh.HasPositions() || !importedMesh.HasFaces())
			{
				continue;
			}

			std::vector<Vertex> vertices;
			vertices.reserve(importedMesh.mNumVertices);

			for (uint32_t vertexIndex = 0; vertexIndex < importedMesh.mNumVertices; ++vertexIndex)
			{
				Vertex vertex{};
				vertex.Position = { importedMesh.mVertices[vertexIndex].x, importedMesh.mVertices[vertexIndex].y, importedMesh.mVertices[vertexIndex].z };
				vertex.Normal = importedMesh.HasNormals() ? glm::vec3{ importedMesh.mNormals[vertexIndex].x, importedMesh.mNormals[vertexIndex].y, importedMesh.mNormals[vertexIndex].z } : glm::vec3{ 0.0f, 0.0f, 1.0f };
				vertex.Color = importedMesh.HasVertexColors(0) ? glm::vec3{ importedMesh.mColors[0][vertexIndex].r, importedMesh.mColors[0][vertexIndex].g, importedMesh.mColors[0][vertexIndex].b } : glm::vec3{ 1.0f };
				vertex.UV = importedMesh.HasTextureCoords(0) ? glm::vec2{ importedMesh.mTextureCoords[0][vertexIndex].x, importedMesh.mTextureCoords[0][vertexIndex].y } : glm::vec2{ 0.0f };

				vertices.push_back(vertex);
			}

			std::vector<uint32_t> indices;
			indices.reserve(static_cast<size_t>(importedMesh.mNumFaces) * 3);

			for (uint32_t faceIndex = 0; faceIndex < importedMesh.mNumFaces; ++faceIndex)
			{
				const aiFace& face = importedMesh.mFaces[faceIndex];

				if (face.mNumIndices != 3)
				{
					continue;
				}

				indices.push_back(face.mIndices[0]);
				indices.push_back(face.mIndices[1]);
				indices.push_back(face.mIndices[2]);
			}

			if (indices.empty())
			{
				continue;
			}

			const std::shared_ptr<Mesh> mesh = renderer.CreateMesh(vertices, indices);

			if (!mesh)
			{
				continue;
			}

			Material material{};

			if (importedMesh.mMaterialIndex < importedScene->mNumMaterials)
			{
				const aiMaterial& importedMaterial = *importedScene->mMaterials[importedMesh.mMaterialIndex];

				material.Albedo = LoadMaterialTexture(renderer, *importedScene, importedMaterial, modelDirectory, textureCache);

				aiColor4D baseColor{ 1.0f, 1.0f, 1.0f, 1.0f };

				if (importedMaterial.Get(AI_MATKEY_BASE_COLOR, baseColor) == aiReturn_SUCCESS || importedMaterial.Get(AI_MATKEY_COLOR_DIFFUSE, baseColor) == aiReturn_SUCCESS)
				{
					material.Tint = { baseColor.r, baseColor.g, baseColor.b, baseColor.a };
				}

				int twoSided = 0;

				if (importedMaterial.Get(AI_MATKEY_TWOSIDED, twoSided) == aiReturn_SUCCESS)
				{
					material.TwoSided = twoSided != 0;
				}
			}

			std::string name = importedMesh.mName.C_Str();

			if (name.empty())
			{
				name = "Mesh " + std::to_string(meshIndex);
			}

			Entity entity = scene.CreateEntity(name);
			entity.AddMeshRenderer(mesh, material);

			entities.push_back(entity);
		}

		IG_CORE_INFO("------- MODEL IMPORTED: {} MESHES -------", entities.size());

		return entities;
	}
}