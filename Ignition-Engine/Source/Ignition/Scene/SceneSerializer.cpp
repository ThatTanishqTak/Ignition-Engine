#include "Ignition/Scene/SceneSerializer.h"

#include "Ignition/Assets/AssetRegistry.h"
#include "Ignition/Core/Log.h"
#include "Ignition/Renderer/Material.h"
#include "Ignition/Scene/Components.h"
#include "Ignition/Scene/Entity.h"
#include "Ignition/Scene/Scene.h"

#include <yaml-cpp/yaml.h>

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <filesystem>
#include <fstream>

namespace YAML
{
	template<>
	struct convert<glm::vec3>
	{
		static Node encode(const glm::vec3& value)
		{
			Node node;
			node.push_back(value.x);
			node.push_back(value.y);
			node.push_back(value.z);
			node.SetStyle(EmitterStyle::Flow);

			return node;
		}

		static bool decode(const Node& node, glm::vec3& value)
		{
			if (!node.IsSequence() || node.size() != 3)
			{
				return false;
			}

			value = { node[0].as<float>(), node[1].as<float>(), node[2].as<float>() };

			return true;
		}
	};

	template<>
	struct convert<glm::vec4>
	{
		static Node encode(const glm::vec4& value)
		{
			Node node;
			node.push_back(value.x);
			node.push_back(value.y);
			node.push_back(value.z);
			node.push_back(value.w);
			node.SetStyle(EmitterStyle::Flow);

			return node;
		}

		static bool decode(const Node& node, glm::vec4& value)
		{
			if (!node.IsSequence() || node.size() != 4)
			{
				return false;
			}

			value = { node[0].as<float>(), node[1].as<float>(), node[2].as<float>(), node[3].as<float>() };

			return true;
		}
	};
}

namespace Ignition
{
	namespace
	{
		YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec3& value)
		{
			out << YAML::Flow << YAML::BeginSeq << value.x << value.y << value.z << YAML::EndSeq;

			return out;
		}

		YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec4& value)
		{
			out << YAML::Flow << YAML::BeginSeq << value.x << value.y << value.z << value.w << YAML::EndSeq;

			return out;
		}
	}

	SceneSerializer::SceneSerializer(Scene* scene, AssetRegistry* assets) : m_Scene(scene), m_Assets(assets)
	{

	}

	bool SceneSerializer::Save(const std::string& filepath) const
	{
		YAML::Emitter out;

		out << YAML::BeginMap;
		out << YAML::Key << "Scene" << YAML::Value << "Untitled";
		out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

		for (Entity entity : m_Scene->GetEntities())
		{
			out << YAML::BeginMap;
			out << YAML::Key << "Name" << YAML::Value << entity.GetName();

			const TransformComponent& transform = entity.GetTransform();

			out << YAML::Key << "Transform" << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "Position" << YAML::Value << transform.Position;
			out << YAML::Key << "Rotation" << YAML::Value << transform.Rotation; // radians
			out << YAML::Key << "Scale" << YAML::Value << transform.Scale;
			out << YAML::EndMap;

			if (const MeshRendererComponent* meshRenderer = entity.GetMeshRenderer())
			{
				out << YAML::Key << "MeshRenderer" << YAML::Value << YAML::BeginMap;
				out << YAML::Key << "Mesh" << YAML::Value << meshRenderer->MeshAsset;
				out << YAML::Key << "Albedo" << YAML::Value << meshRenderer->AlbedoAsset;
				out << YAML::Key << "Tint" << YAML::Value << meshRenderer->Material.Tint;
				out << YAML::Key << "TwoSided" << YAML::Value << meshRenderer->Material.TwoSided;
				out << YAML::EndMap;
			}

			out << YAML::EndMap;
		}

		out << YAML::EndSeq;
		out << YAML::EndMap;

		std::error_code errorCode;
		std::filesystem::create_directories(std::filesystem::path(filepath).parent_path(), errorCode);

		std::ofstream file(filepath);

		if (!file.is_open())
		{
			return false;
		}

		file << out.c_str();

		return true;
	}

	bool SceneSerializer::Load(const std::string& filepath) const
	{
		YAML::Node root;

		try
		{
			root = YAML::LoadFile(filepath);
		}
		catch (const YAML::Exception& exception)
		{
			IG_CORE_ERROR("Scene load failed: {}", exception.what());

			return false;
		}

		if (!root["Entities"])
		{
			return false;
		}

		// Replace the current scene contents wholesale
		for (Entity entity : m_Scene->GetEntities())
		{
			m_Scene->DestroyEntity(entity);
		}

		for (const YAML::Node& entityNode : root["Entities"])
		{
			Entity entity = m_Scene->CreateEntity(entityNode["Name"].as<std::string>("Entity"));

			if (const YAML::Node transformNode = entityNode["Transform"])
			{
				TransformComponent& transform = entity.GetTransform();
				transform.Position = transformNode["Position"].as<glm::vec3>(glm::vec3(0.0f));
				transform.Rotation = transformNode["Rotation"].as<glm::vec3>(glm::vec3(0.0f));
				transform.Scale = transformNode["Scale"].as<glm::vec3>(glm::vec3(1.0f));
			}

			if (const YAML::Node meshRendererNode = entityNode["MeshRenderer"])
			{
				const std::string meshAsset = meshRendererNode["Mesh"].as<std::string>("");
				const std::string albedoAsset = meshRendererNode["Albedo"].as<std::string>("");

				Material material;
				material.Tint = meshRendererNode["Tint"].as<glm::vec4>(glm::vec4(1.0f));
				material.TwoSided = meshRendererNode["TwoSided"].as<bool>(false);
				material.Albedo = m_Assets->LoadTexture(albedoAsset);

				MeshRendererComponent& meshRenderer = entity.AddMeshRenderer(m_Assets->LoadMesh(meshAsset), material);
				meshRenderer.MeshAsset = meshAsset;
				meshRenderer.AlbedoAsset = albedoAsset;
			}
		}

		return true;
	}
}