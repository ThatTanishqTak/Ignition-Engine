#pragma once

#include <glm/vec3.hpp>

namespace Ignition
{
	class Camera;
	class Input;
	class ActionMap;
}

namespace Sandbox
{
	class CameraController
	{
	public:
		CameraController(Ignition::Input* input, Ignition::ActionMap* actions);

		void Update(Ignition::Camera& camera, float deltaTime, bool uiWantsMouse, bool uiWantsKeyboard);

	private:
		glm::vec3 GetForward() const;

	private:
		Ignition::Input* m_Input = nullptr;
		Ignition::ActionMap* m_Actions = nullptr;

		glm::vec3 m_Position{ 0.0f, 0.0f, 2.0f };
		float m_Yaw = 0.0f;
		float m_Pitch = 0.0f;

		float m_MoveSpeed = 3.0f;
		float m_LookSensitivity = 0.002f;

		bool m_Looking = false;
	};
}