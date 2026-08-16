#include "Ignition/Scene/ModelImporter.h"

#include "Ignition/Assets/AssetRegistry.h"
#include "Ignition/Core/Log.h"
#include "Ignition/Renderer/Material.h"
#include "Ignition/Scene/Components.h"
#include "Ignition/Scene/ModelLoader.h"
#include "Ignition/Scene/Scene.h"

#include <string>

namespace Ignition
{
	std::vector<Entity> ModelImporter::Import(Scene& scene, AssetRegistry& assets, const std::string& filepath)
	{
		IG_CORE_INFO("------- IMPORTING MODEL '{}' -------", filepath);

		const ModelData* model = assets.LoadModel(filepath);

		if (!model)
		{
			return {};
		}

		std::vector<Entity> entities;
		entities.reserve(model->Submeshes.size());

		for (size_t submeshIndex = 0; submeshIndex < model->Submeshes.size(); ++submeshIndex)
		{
			const ModelSubmesh& submesh = model->Submeshes[submeshIndex];

			// Every reference is a path, so the imported scene survives a save/load round trip
			const std::string meshAsset = filepath + "#" + std::to_string(submeshIndex);

			std::string albedoAsset = submesh.AlbedoPath;

			if (albedoAsset.empty() && submesh.EmbeddedAlbedoIndex >= 0)
			{
				albedoAsset = filepath + "#tex:" + std::to_string(submesh.EmbeddedAlbedoIndex);
			}

			const std::shared_ptr<Mesh> mesh = assets.LoadMesh(meshAsset);

			if (!mesh)
			{
				continue;
			}

			Material material{};
			material.Tint = submesh.Tint;
			material.TwoSided = submesh.TwoSided;
			material.Albedo = assets.LoadTexture(albedoAsset);

			Entity entity = scene.CreateEntity(submesh.Name);

			MeshRendererComponent& meshRenderer = entity.AddMeshRenderer(mesh, material);
			meshRenderer.MeshAsset = meshAsset;
			meshRenderer.AlbedoAsset = albedoAsset;

			entities.push_back(entity);
		}

		IG_CORE_INFO("------- MODEL IMPORTED: {} MESHES -------", entities.size());

		return entities;
	}
}