#pragma once

#include "Ignition/Core/Export.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace Ignition
{
	class Mesh;
	class Renderer;
	class Texture;

	class AssetRegistry
	{
	public:
		IGNITION_API explicit AssetRegistry(Renderer* renderer);
		IGNITION_API ~AssetRegistry();

		AssetRegistry(const AssetRegistry&) = delete;
		AssetRegistry& operator=(const AssetRegistry&) = delete;

		IGNITION_API std::shared_ptr<Mesh> LoadMesh(const std::string& path);
		IGNITION_API std::shared_ptr<Texture> LoadTexture(const std::string& path);

	private:
		Renderer* m_Renderer = nullptr;

		std::unordered_map<std::string, std::shared_ptr<Mesh>> m_Meshes;
		std::unordered_map<std::string, std::shared_ptr<Texture>> m_Textures;
	};
}