#include "Ignition/Scene/SceneSerializer.h"

#include "Ignition/Assets/AssetRegistry.h"
#include "Ignition/Core/Log.h"
#include "Ignition/Physics/PhysicsComponents.h"
#include "Ignition/Renderer/Material.h"
#include "Ignition/Scene/Components.h"
#include "Ignition/Scene/Entity.h"
#include "Ignition/Scene/Scene.h"

#include <yaml-cpp/yaml.h>

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

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
	struct convert<glm::uvec3>
	{
		static Node encode(const glm::uvec3& value)
		{
			Node node;
			node.push_back(value.x);
			node.push_back(value.y);
			node.push_back(value.z);
			node.SetStyle(EmitterStyle::Flow);

			return node;
		}

		static bool decode(const Node& node, glm::uvec3& value)
		{
			if (!node.IsSequence() || node.size() != 3)
			{
				return false;
			}

			value = { node[0].as<uint32_t>(), node[1].as<uint32_t>(), node[2].as<uint32_t>() };

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

		YAML::Emitter& operator<<(YAML::Emitter& out, const glm::uvec3& value)
		{
			out << YAML::Flow << YAML::BeginSeq << value.x << value.y << value.z << YAML::EndSeq;

			return out;
		}

		YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec4& value)
		{
			out << YAML::Flow << YAML::BeginSeq << value.x << value.y << value.z << value.w << YAML::EndSeq;

			return out;
		}

		YAML::Emitter& operator<<(YAML::Emitter& out, const glm::quat& value)
		{
			out << YAML::Flow << YAML::BeginSeq << value.x << value.y << value.z << value.w << YAML::EndSeq;

			return out;
		}

		// Four components are a quaternion; three are Euler radians from a scene written before the switch
		glm::quat ReadRotation(const YAML::Node& node)
		{
			if (node && node.IsSequence() && node.size() == 4)
			{
				return glm::normalize(glm::quat(node[3].as<float>(), node[0].as<float>(), node[1].as<float>(), node[2].as<float>()));
			}

			if (node && node.IsSequence() && node.size() == 3)
			{
				return glm::normalize(glm::quat(glm::vec3(node[0].as<float>(), node[1].as<float>(), node[2].as<float>())));
			}

			return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
		}
	}

	SceneSerializer::SceneSerializer(Scene* scene, AssetRegistry* assets) : m_Scene(scene), m_Assets(assets)
	{

	}

	bool SceneSerializer::Save(const std::string& filepath) const
	{
		const std::string data = SaveToString();

		if (data.empty())
		{
			return false;
		}

		std::error_code errorCode;
		std::filesystem::create_directories(std::filesystem::path(filepath).parent_path(), errorCode);

		std::ofstream file(filepath);

		if (!file.is_open())
		{
			return false;
		}

		file << data;

		return true;
	}

	std::string SceneSerializer::SaveToString() const
	{
		YAML::Emitter out;

		const FluidSolver3DSettings& tunnel = m_Scene->GetWindTunnel();

		out << YAML::BeginMap;
		out << YAML::Key << "Scene" << YAML::Value << "Untitled";

		out << YAML::Key << "WindTunnel" << YAML::Value << YAML::BeginMap;
		out << YAML::Key << "Resolution" << YAML::Value << tunnel.Resolution;
		out << YAML::Key << "DomainSize" << YAML::Value << tunnel.DomainSize;
		out << YAML::Key << "ReferencePoint" << YAML::Value << tunnel.ReferencePoint;
		out << YAML::Key << "InletSpeed" << YAML::Value << tunnel.InletSpeed;
		out << YAML::Key << "AirDensity" << YAML::Value << tunnel.AirDensity;
		out << YAML::Key << "KinematicViscosity" << YAML::Value << tunnel.KinematicViscosity;
		out << YAML::Key << "SmagorinskyConstant" << YAML::Value << tunnel.SmagorinskyConstant;
		out << YAML::Key << "LatticeVelocity" << YAML::Value << tunnel.LatticeVelocity;
		out << YAML::Key << "MinimumRelaxationTime" << YAML::Value << tunnel.MinimumRelaxationTime;
		out << YAML::Key << "ReferenceLength" << YAML::Value << tunnel.ReferenceLength;
		out << YAML::Key << "Wheelbase" << YAML::Value << tunnel.Wheelbase;
		out << YAML::Key << "RollingRoad" << YAML::Value << tunnel.RollingRoad;
		out << YAML::Key << "VolumeEnabled" << YAML::Value << tunnel.VolumeEnabled;
		out << YAML::Key << "VolumeField" << YAML::Value << static_cast<int>(tunnel.VolumeField);
		out << YAML::Key << "VolumeDensity" << YAML::Value << tunnel.VolumeDensity;
		out << YAML::Key << "VolumeThreshold" << YAML::Value << tunnel.VolumeThreshold;
		out << YAML::Key << "VolumeSteps" << YAML::Value << tunnel.VolumeSteps;
		out << YAML::Key << "SliceEnabled" << YAML::Value << tunnel.SliceEnabled;
		out << YAML::Key << "SliceAxis" << YAML::Value << static_cast<int>(tunnel.SliceAxis);
		out << YAML::Key << "SlicePosition" << YAML::Value << tunnel.SlicePosition;
		out << YAML::Key << "SliceField" << YAML::Value << static_cast<int>(tunnel.SliceField);
		out << YAML::Key << "ColorScale" << YAML::Value << tunnel.ColorScale;
		out << YAML::Key << "SliceOpacity" << YAML::Value << tunnel.SliceOpacity;
		out << YAML::Key << "ParticlesEnabled" << YAML::Value << tunnel.ParticlesEnabled;
		out << YAML::Key << "ParticleCount" << YAML::Value << tunnel.ParticleCount;
		out << YAML::Key << "SurfacePressureEnabled" << YAML::Value << tunnel.SurfacePressureEnabled;
		out << YAML::Key << "VoxelDebugView" << YAML::Value << tunnel.VoxelDebugView;
		out << YAML::Key << "FloodIterations" << YAML::Value << tunnel.FloodIterations;
		out << YAML::EndMap;

		out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

		for (Entity entity : m_Scene->GetEntities())
		{
			out << YAML::BeginMap;
			out << YAML::Key << "Name" << YAML::Value << entity.GetName();

			const TransformComponent& transform = entity.GetTransform();

			out << YAML::Key << "Transform" << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "Position" << YAML::Value << transform.Position;
			out << YAML::Key << "Rotation" << YAML::Value << transform.Rotation; // quaternion, x y z w
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

			if (const RigidBodyComponent* rigidBody = entity.GetRigidBody())
			{
				out << YAML::Key << "RigidBody" << YAML::Value << YAML::BeginMap;
				out << YAML::Key << "Type" << YAML::Value << static_cast<int>(rigidBody->Type);
				out << YAML::Key << "Mass" << YAML::Value << rigidBody->Mass;
				out << YAML::Key << "LinearDamping" << YAML::Value << rigidBody->LinearDamping;
				out << YAML::Key << "AngularDamping" << YAML::Value << rigidBody->AngularDamping;
				out << YAML::Key << "UseGravity" << YAML::Value << rigidBody->UseGravity;
				out << YAML::EndMap;
			}

			if (const PhysicsMaterialComponent* physicsMaterial = entity.GetPhysicsMaterial())
			{
				out << YAML::Key << "PhysicsMaterial" << YAML::Value << YAML::BeginMap;
				out << YAML::Key << "StaticFriction" << YAML::Value << physicsMaterial->StaticFriction;
				out << YAML::Key << "DynamicFriction" << YAML::Value << physicsMaterial->DynamicFriction;
				out << YAML::Key << "Restitution" << YAML::Value << physicsMaterial->Restitution;
				out << YAML::EndMap;
			}

			if (const BoxColliderComponent* collider = entity.GetBoxCollider())
			{
				out << YAML::Key << "BoxCollider" << YAML::Value << YAML::BeginMap;
				out << YAML::Key << "HalfExtents" << YAML::Value << collider->HalfExtents;
				out << YAML::Key << "Offset" << YAML::Value << collider->Offset;
				out << YAML::EndMap;
			}

			if (const SphereColliderComponent* collider = entity.GetSphereCollider())
			{
				out << YAML::Key << "SphereCollider" << YAML::Value << YAML::BeginMap;
				out << YAML::Key << "Radius" << YAML::Value << collider->Radius;
				out << YAML::Key << "Offset" << YAML::Value << collider->Offset;
				out << YAML::EndMap;
			}

			if (const CapsuleColliderComponent* collider = entity.GetCapsuleCollider())
			{
				out << YAML::Key << "CapsuleCollider" << YAML::Value << YAML::BeginMap;
				out << YAML::Key << "Radius" << YAML::Value << collider->Radius;
				out << YAML::Key << "HalfHeight" << YAML::Value << collider->HalfHeight;
				out << YAML::Key << "Offset" << YAML::Value << collider->Offset;
				out << YAML::EndMap;
			}

			if (const MeshColliderComponent* collider = entity.GetMeshCollider())
			{
				out << YAML::Key << "MeshCollider" << YAML::Value << YAML::BeginMap;
				out << YAML::Key << "Mesh" << YAML::Value << collider->MeshAsset;
				out << YAML::Key << "Convex" << YAML::Value << collider->Convex;
				out << YAML::Key << "Offset" << YAML::Value << collider->Offset;
				out << YAML::EndMap;
			}

			out << YAML::EndMap;
		}

		out << YAML::EndSeq;
		out << YAML::EndMap;

		return out.c_str();
	}

	bool SceneSerializer::Load(const std::string& filepath) const
	{
		std::ifstream file(filepath);

		if (!file.is_open())
		{
			IG_CORE_ERROR("Scene load failed: could not open '{}'", filepath);

			return false;
		}

		return LoadFromString(std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()));
	}

	bool SceneSerializer::LoadFromString(const std::string& data) const
	{
		YAML::Node root;

		try
		{
			root = YAML::Load(data);
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

		// Absent in a pre-Phase-3 scene file, which is exactly what the defaults are for
		if (const YAML::Node tunnelNode = root["WindTunnel"])
		{
			FluidSolver3DSettings tunnel;
			tunnel.Resolution = tunnelNode["Resolution"].as<glm::uvec3>(glm::uvec3(128, 64, 256));
			tunnel.DomainSize = tunnelNode["DomainSize"].as<glm::vec3>(glm::vec3(4.0f, 2.0f, 8.0f));
			tunnel.ReferencePoint = tunnelNode["ReferencePoint"].as<glm::vec3>(glm::vec3(0.0f));
			tunnel.InletSpeed = tunnelNode["InletSpeed"].as<float>(40.0f);
			tunnel.AirDensity = tunnelNode["AirDensity"].as<float>(1.225f);
			tunnel.KinematicViscosity = tunnelNode["KinematicViscosity"].as<float>(1.48e-5f);
			tunnel.SmagorinskyConstant = tunnelNode["SmagorinskyConstant"].as<float>(0.16f);
			tunnel.LatticeVelocity = tunnelNode["LatticeVelocity"].as<float>(0.06f);
			tunnel.MinimumRelaxationTime = tunnelNode["MinimumRelaxationTime"].as<float>(0.503f);
			tunnel.ReferenceLength = tunnelNode["ReferenceLength"].as<float>(1.0f);
			tunnel.Wheelbase = tunnelNode["Wheelbase"].as<float>(3.6f);
			tunnel.RollingRoad = tunnelNode["RollingRoad"].as<bool>(true);
			tunnel.VolumeEnabled = tunnelNode["VolumeEnabled"].as<bool>(true);
			tunnel.VolumeField = static_cast<FluidField>(std::clamp(tunnelNode["VolumeField"].as<int>(1), 0, 2));
			tunnel.VolumeDensity = tunnelNode["VolumeDensity"].as<float>(4.0f);
			tunnel.VolumeThreshold = tunnelNode["VolumeThreshold"].as<float>(0.08f);
			tunnel.VolumeSteps = tunnelNode["VolumeSteps"].as<uint32_t>(256);

			// A scene written before the volume existed keeps its plane; one written after gets whatever it was saved with
			tunnel.SliceEnabled = tunnelNode["SliceEnabled"].as<bool>(false);
			tunnel.SliceAxis = static_cast<FluidSliceAxis>(std::clamp(tunnelNode["SliceAxis"].as<int>(0), 0, 2));
			tunnel.SlicePosition = tunnelNode["SlicePosition"].as<float>(0.5f);
			tunnel.SliceField = static_cast<FluidField>(std::clamp(tunnelNode["SliceField"].as<int>(0), 0, 2));
			tunnel.ColorScale = tunnelNode["ColorScale"].as<float>(1.0f);
			tunnel.SliceOpacity = tunnelNode["SliceOpacity"].as<float>(0.85f);
			tunnel.ParticlesEnabled = tunnelNode["ParticlesEnabled"].as<bool>(true);
			tunnel.ParticleCount = tunnelNode["ParticleCount"].as<uint32_t>(100000);
			tunnel.SurfacePressureEnabled = tunnelNode["SurfacePressureEnabled"].as<bool>(false);
			tunnel.VoxelDebugView = tunnelNode["VoxelDebugView"].as<bool>(false);
			tunnel.FloodIterations = tunnelNode["FloodIterations"].as<uint32_t>(8);

			m_Scene->GetWindTunnel() = tunnel;
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
				transform.Rotation = ReadRotation(transformNode["Rotation"]);
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

			if (const YAML::Node rigidBodyNode = entityNode["RigidBody"])
			{
				RigidBodyComponent rigidBody;
				rigidBody.Type = static_cast<RigidBodyType>(rigidBodyNode["Type"].as<int>(static_cast<int>(RigidBodyType::Dynamic)));
				rigidBody.Mass = rigidBodyNode["Mass"].as<float>(1.0f);
				rigidBody.LinearDamping = rigidBodyNode["LinearDamping"].as<float>(0.01f);
				rigidBody.AngularDamping = rigidBodyNode["AngularDamping"].as<float>(0.05f);
				rigidBody.UseGravity = rigidBodyNode["UseGravity"].as<bool>(true);

				entity.AddRigidBody(rigidBody);
			}

			if (const YAML::Node materialNode = entityNode["PhysicsMaterial"])
			{
				PhysicsMaterialComponent physicsMaterial;
				physicsMaterial.StaticFriction = materialNode["StaticFriction"].as<float>(0.6f);
				physicsMaterial.DynamicFriction = materialNode["DynamicFriction"].as<float>(0.6f);
				physicsMaterial.Restitution = materialNode["Restitution"].as<float>(0.0f);

				entity.AddPhysicsMaterial(physicsMaterial);
			}

			if (const YAML::Node colliderNode = entityNode["BoxCollider"])
			{
				BoxColliderComponent collider;
				collider.HalfExtents = colliderNode["HalfExtents"].as<glm::vec3>(glm::vec3(0.5f));
				collider.Offset = colliderNode["Offset"].as<glm::vec3>(glm::vec3(0.0f));

				entity.AddBoxCollider(collider);
			}

			if (const YAML::Node colliderNode = entityNode["SphereCollider"])
			{
				SphereColliderComponent collider;
				collider.Radius = colliderNode["Radius"].as<float>(0.5f);
				collider.Offset = colliderNode["Offset"].as<glm::vec3>(glm::vec3(0.0f));

				entity.AddSphereCollider(collider);
			}

			if (const YAML::Node colliderNode = entityNode["CapsuleCollider"])
			{
				CapsuleColliderComponent collider;
				collider.Radius = colliderNode["Radius"].as<float>(0.5f);
				collider.HalfHeight = colliderNode["HalfHeight"].as<float>(0.5f);
				collider.Offset = colliderNode["Offset"].as<glm::vec3>(glm::vec3(0.0f));

				entity.AddCapsuleCollider(collider);
			}

			if (const YAML::Node colliderNode = entityNode["MeshCollider"])
			{
				MeshColliderComponent collider;
				collider.MeshAsset = colliderNode["Mesh"].as<std::string>("");
				collider.Convex = colliderNode["Convex"].as<bool>(true);
				collider.Offset = colliderNode["Offset"].as<glm::vec3>(glm::vec3(0.0f));

				entity.AddMeshCollider(collider);
			}
		}

		return true;
	}
}