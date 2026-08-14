#pragma once

#include <glm/vec3.hpp>

namespace Ignition
{
	class Camera;
	class Input;
}

namespace Editor
{
	struct EditorContext;

	class EditorCameraController
	{
	public:
		explicit EditorCameraController(Ignition::Input* input);

		void Update(Ignition::Camera& camera, float deltaTime, const EditorContext& context);

		void Focus(const glm::vec3& point, float radius);

	private:
		glm::vec3 GetForward() const;
		glm::vec3 GetRight() const;

	private:
		Ignition::Input* m_Input = nullptr;

		glm::vec3 m_Position{ 0.0f, 1.0f, 4.0f };
		float m_Yaw = 0.0f;
		float m_Pitch = 0.0f;

		float m_MoveSpeed = 5.0f;
		float m_LookSensitivity = 0.002f;
		float m_OrbitDistance = 5.0f;

		bool m_Looking = false;
		bool m_Orbiting = false;
		bool m_Panning = false;
	};
}