#include "Ignition/Assets/AssetRegistry.h"

#include "Ignition/Core/Log.h"
#include "Ignition/Renderer/Renderer.h"
#include "Ignition/Renderer/Vertex.h"
#include "Ignition/Scene/ModelLoader.h"

#include <charconv>
#include <string_view>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/common.hpp>
#include <glm/geometric.hpp>

#include <array>
#include <vector>

namespace Ignition
{
	namespace
	{
		void BuildQuad(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices)
		{
			vertices = {
				{ { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } },
				{ {  0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } },
				{ {  0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f } },
				{ { -0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f } },
			};

			indices = { 0, 1, 2, 2, 3, 0 };
		}

		void BuildCube(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices)
		{
			const std::array<glm::vec3, 6> normals = { {
				{ 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, -1.0f },
				{ 1.0f, 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f },
				{ 0.0f, 1.0f, 0.0f }, { 0.0f, -1.0f, 0.0f },
			} };

			for (const glm::vec3& normal : normals)
			{
				const glm::vec3 tangent = glm::abs(normal.y) > 0.5f ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), normal);
				const glm::vec3 bitangent = glm::cross(normal, tangent);
				const uint32_t base = static_cast<uint32_t>(vertices.size());

				for (int corner = 0; corner < 4; corner++)
				{
					const float u = (corner == 1 || corner == 2) ? 1.0f : 0.0f;
					const float v = (corner >= 2) ? 1.0f : 0.0f;
					const glm::vec3 position = normal * 0.5f + tangent * (u - 0.5f) + bitangent * (v - 0.5f);

					vertices.push_back({ position, normal, glm::vec3(1.0f), glm::vec2(u, v) });
				}

				indices.insert(indices.end(), { base, base + 1, base + 2, base + 2, base + 3, base });
			}
		}

		bool SplitAssetReference(const std::string& path, std::string& outFile, std::string& outFragment)
		{
			const size_t separator = path.find('#');

			if (separator == std::string::npos)
			{
				outFile = path;
				outFragment.clear();

				return false;
			}

			outFile = path.substr(0, separator);
			outFragment = path.substr(separator + 1);

			return true;
		}

		int ParseIndex(std::string_view text)
		{
			int value = -1;
			const auto result = std::from_chars(text.data(), text.data() + text.size(), value);

			return (result.ec == std::errc() && result.ptr == text.data() + text.size()) ? value : -1;
		}

		void AppendSubmesh(const ModelSubmesh& submesh, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices)
		{
			const uint32_t base = static_cast<uint32_t>(vertices.size());

			vertices.insert(vertices.end(), submesh.Vertices.begin(), submesh.Vertices.end());

			for (uint32_t index : submesh.Indices)
			{
				indices.push_back(base + index);
			}
		}
	}

	AssetRegistry::AssetRegistry(Renderer* renderer) : m_Renderer(renderer)
	{

	}

	AssetRegistry::~AssetRegistry() = default;

	const ModelData* AssetRegistry::LoadModel(const std::string& filepath)
	{
		if (const auto it = m_Models.find(filepath); it != m_Models.end())
		{
			return it->second.get();
		}

		auto model = std::make_unique<ModelData>();

		if (!LoadModelFile(filepath, *model))
		{
			m_Models.emplace(filepath, nullptr);

			return nullptr;
		}

		const ModelData* loaded = model.get();
		m_Models.emplace(filepath, std::move(model));

		return loaded;
	}

	std::shared_ptr<Mesh> AssetRegistry::LoadMesh(const std::string& path)
	{
		if (path.empty())
		{
			return nullptr;
		}

		if (const auto it = m_Meshes.find(path); it != m_Meshes.end())
		{
			return it->second;
		}

		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;

		if (path == "builtin:quad")
		{
			BuildQuad(vertices, indices);
		}
		else if (path == "builtin:cube")
		{
			BuildCube(vertices, indices);
		}
		else if (path.starts_with("builtin:"))
		{
			IG_CORE_ERROR("AssetRegistry: unknown builtin mesh '{}'", path);

			return nullptr;
		}
		else
		{
			std::string filepath;
			std::string fragment;
			SplitAssetReference(path, filepath, fragment);

			const ModelData* model = LoadModel(filepath);

			if (!model)
			{
				return nullptr;
			}

			if (fragment.empty())
			{
				for (const ModelSubmesh& submesh : model->Submeshes)
				{
					AppendSubmesh(submesh, vertices, indices);
				}
			}
			else
			{
				const int submeshIndex = ParseIndex(fragment);

				if (submeshIndex < 0 || static_cast<size_t>(submeshIndex) >= model->Submeshes.size())
				{
					IG_CORE_ERROR("AssetRegistry: mesh reference '{}' is out of range ({} submeshes)", path, model->Submeshes.size());

					return nullptr;
				}

				AppendSubmesh(model->Submeshes[submeshIndex], vertices, indices);
			}
		}

		if (indices.empty())
		{
			IG_CORE_ERROR("AssetRegistry: mesh asset '{}' produced no geometry", path);

			return nullptr;
		}

		auto mesh = m_Renderer->CreateMesh(vertices, indices);

		if (mesh)
		{
			m_Meshes.emplace(path, mesh);
		}

		return mesh;
	}

	std::shared_ptr<Texture> AssetRegistry::LoadTexture(const std::string& path)
	{
		if (path.empty())
		{
			return nullptr;
		}

		if (const auto it = m_Textures.find(path); it != m_Textures.end())
		{
			return it->second;
		}

		std::string filepath;
		std::string fragment;

		std::shared_ptr<Texture> texture;

		if (SplitAssetReference(path, filepath, fragment) && fragment.starts_with("tex:"))
		{
			const ModelData* model = LoadModel(filepath);
			const int textureIndex = ParseIndex(std::string_view(fragment).substr(4));

			if (!model || textureIndex < 0 || static_cast<size_t>(textureIndex) >= model->EmbeddedTextures.size())
			{
				IG_CORE_ERROR("AssetRegistry: embedded texture reference '{}' could not be resolved", path);

				return nullptr;
			}

			const std::vector<uint8_t>& data = model->EmbeddedTextures[textureIndex].Data;

			if (data.empty())
			{
				return nullptr;
			}

			texture = m_Renderer->CreateTextureFromMemory(data.data(), data.size());
		}
		else
		{
			texture = m_Renderer->CreateTexture(path);
		}

		if (texture)
		{
			m_Textures.emplace(path, texture);
		}

		return texture;
	}
}