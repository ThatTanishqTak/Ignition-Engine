#pragma once

#include "Ignition/Renderer/Material.h"

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <memory>
#include <string>

namespace Ignition
{
	class Mesh;

	struct TagComponent
	{
		std::string Name;
	};

	struct TransformComponent
	{
		glm::vec3 Position{ 0.0f };
		glm::quat Rotation{ 1.0f, 0.0f, 0.0f, 0.0f }; // identity, stored w-first
		glm::vec3 Scale{ 1.0f };

		glm::mat4 GetMatrix() const
		{
			return glm::translate(glm::mat4(1.0f), Position) * glm::mat4_cast(Rotation) * glm::scale(glm::mat4(1.0f), Scale);
		}

		// Euler angles are a presentation detail: convert at the UI boundary, never store them
		glm::vec3 GetEulerAngles() const
		{
			return glm::eulerAngles(Rotation);
		}

		void SetEulerAngles(const glm::vec3& radians)
		{
			Rotation = glm::normalize(glm::quat(radians));
		}
	};

	struct MeshRendererComponent
	{
		std::shared_ptr<Ignition::Mesh> Mesh;
		Ignition::Material Material;

		std::string MeshAsset;
		std::string AlbedoAsset;
	};
}