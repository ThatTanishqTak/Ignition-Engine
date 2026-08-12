#pragma once

#include "Ignition/Input/ActionMap.h"
#include "Ignition/Input/Input.h"
#include "Ignition/Renderer/Camera.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

namespace Sandbox
{
	class CameraController
	{
	public:
		CameraController(Ignition::Input* input, Ignition::ActionMap* actions) : m_Input(input), m_Actions(actions)
		{

		}

		void Update(Ignition::Camera& camera, float deltaTime, bool uiWantsMouse, bool uiWantsKeyboard)
		{
			const bool lookHeld = m_Input->IsMouseButtonDown(Ignition::MouseCode::RIGHT);

			if (lookHeld && !m_Looking && !uiWantsMouse)
			{
				m_Input->SetCursorMode(Ignition::CursorMode::Captured);
				m_Looking = true;
			}
			else if (!lookHeld && m_Looking)
			{
				m_Input->SetCursorMode(Ignition::CursorMode::Normal);
				m_Looking = false;
			}

			if (m_Looking)
			{
				const glm::vec2 mouseDelta = m_Input->GetMouseDelta();

				m_Yaw += mouseDelta.x * m_LookSensitivity;
				m_Pitch = glm::clamp(m_Pitch - mouseDelta.y * m_LookSensitivity, glm::radians(-89.0f), glm::radians(89.0f));
			}

			const glm::vec3 forward = GetForward();

			if (m_Looking || !uiWantsKeyboard)
			{
				const glm::vec2 move = m_Actions->GetAxis2D("Move");

				if (move.x != 0.0f || move.y != 0.0f)
				{
					const glm::vec3 planarForward = glm::normalize(glm::vec3(forward.x, 0.0f, forward.z));
					const glm::vec3 planarRight = glm::normalize(glm::cross(planarForward, glm::vec3(0.0f, 1.0f, 0.0f)));

					m_Position += (planarForward * move.y + planarRight * move.x) * m_MoveSpeed * deltaTime;
				}
			}

			camera.LookAt(m_Position, m_Position + forward);
		}

	private:
		glm::vec3 GetForward() const
		{
			return glm::vec3(glm::cos(m_Pitch) * glm::sin(m_Yaw), glm::sin(m_Pitch), -glm::cos(m_Pitch) * glm::cos(m_Yaw));
		}

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