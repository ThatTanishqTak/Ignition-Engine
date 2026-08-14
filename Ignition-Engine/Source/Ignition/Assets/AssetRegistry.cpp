#include "Ignition/Assets/AssetRegistry.h"

#include "Ignition/Core/Log.h"
#include "Ignition/Renderer/Renderer.h"
#include "Ignition/Renderer/Vertex.h"

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
	}

	AssetRegistry::AssetRegistry(Renderer* renderer) : m_Renderer(renderer)
	{

	}

	AssetRegistry::~AssetRegistry() = default;

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
		else
		{
			IG_CORE_ERROR("AssetRegistry: unknown mesh asset '{}'", path);

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

		auto texture = m_Renderer->CreateTexture(path);

		if (texture)
		{
			m_Textures.emplace(path, texture);
		}

		return texture;
	}
}