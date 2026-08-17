#pragma once

#include "Ignition/Core/Export.h"

#include <string>

namespace Ignition
{
	class AssetRegistry;
	class Scene;

	class SceneSerializer
	{
	public:
		IGNITION_API SceneSerializer(Scene* scene, AssetRegistry* assets);

		IGNITION_API bool Save(const std::string& filepath) const;
		IGNITION_API bool Load(const std::string& filepath) const;

		// Same emitter/parser as Save/Load - play-mode snapshots cannot drift from what lands on disk
		IGNITION_API std::string SaveToString() const;
		IGNITION_API bool LoadFromString(const std::string& data) const;

	private:
		Scene* m_Scene = nullptr;
		AssetRegistry* m_Assets = nullptr;
	};
}