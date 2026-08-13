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
		glm::vec3 Rotation{ 0.0f }; // Euler angles in radians
		glm::vec3 Scale{ 1.0f };

		glm::mat4 GetMatrix() const
		{
			return glm::translate(glm::mat4(1.0f), Position) * glm::mat4_cast(glm::quat(Rotation)) * glm::scale(glm::mat4(1.0f), Scale);
		}
	};

	struct MeshRendererComponent
	{
		std::shared_ptr<Mesh> Mesh;
		Material Material;
	};
}