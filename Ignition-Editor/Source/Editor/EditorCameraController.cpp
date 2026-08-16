#include "Editor/EditorCameraController.h"

#include "Editor/EditorContext.h"

#include "Ignition/Ignition.h"

#include <glm/vec2.hpp>
#include <glm/common.hpp>
#include <glm/exponential.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

namespace Editor
{
	namespace
	{
		constexpr float MinimumMoveSpeed = 0.1f;
		constexpr float MaximumMoveSpeed = 100.0f;
	}

	EditorCameraController::EditorCameraController(Ignition::Input* input) : m_Input(input)
	{

	}

	void EditorCameraController::Update(Ignition::Camera& camera, float deltaTime, const EditorContext& context)
	{
		const bool canStartAction = context.ViewportHovered && !context.GizmoUsing;

		// Scroll wheel adjusts fly speed on a log scale
		const float wheel = m_Input->GetMouseWheel().y;

		if (canStartAction && wheel != 0.0f)
		{
			m_MoveSpeed = glm::clamp(m_MoveSpeed * glm::pow(1.2f, wheel), MinimumMoveSpeed, MaximumMoveSpeed);
		}

		// F focuses the current selection, framed by its actual mesh bounds
		if (canStartAction && m_Input->IsKeyPressed(Ignition::ScanCode::F) && context.Selection.IsValid())
		{
			const Ignition::TransformComponent& transform = context.Selection.GetTransform();

			float radius = 0.5f;

			if (const Ignition::MeshRendererComponent* meshRenderer = context.Selection.GetMeshRenderer(); meshRenderer && meshRenderer->Mesh)
			{
				const Ignition::MeshBounds bounds = meshRenderer->Mesh->GetBounds();
				const glm::vec3 extent = (bounds.Maximum - bounds.Minimum) * transform.Scale;

				radius = glm::max(glm::length(extent) * 0.5f, 0.01f);
			}

			Focus(transform.Position, radius);
		}

		const bool altHeld = m_Input->IsKeyDown(Ignition::ScanCode::LALT);
		const bool leftHeld = m_Input->IsMouseButtonDown(Ignition::MouseCode::LEFT);
		const bool middleHeld = m_Input->IsMouseButtonDown(Ignition::MouseCode::MIDDLE);
		const bool rightHeld = m_Input->IsMouseButtonDown(Ignition::MouseCode::RIGHT);

		// A mouse mode only begins over the viewport; it ends when its button is released
		if (!m_Looking && !m_Orbiting && !m_Panning && canStartAction)
		{
			if (rightHeld)
			{
				m_Input->SetCursorMode(Ignition::CursorMode::Captured);
				m_Looking = true;
			}
			else if (altHeld && leftHeld)
			{
				m_Input->SetCursorMode(Ignition::CursorMode::Captured);
				m_Orbiting = true;
			}
			else if (middleHeld)
			{
				m_Input->SetCursorMode(Ignition::CursorMode::Captured);
				m_Panning = true;
			}
		}

		if ((m_Looking && !rightHeld) || (m_Orbiting && !leftHeld) || (m_Panning && !middleHeld))
		{
			m_Input->SetCursorMode(Ignition::CursorMode::Normal);
			m_Looking = false;
			m_Orbiting = false;
			m_Panning = false;
		}

		const glm::vec2 mouseDelta = m_Input->GetMouseDelta();

		if (m_Looking)
		{
			m_Yaw += mouseDelta.x * m_LookSensitivity;
			m_Pitch = glm::clamp(m_Pitch - mouseDelta.y * m_LookSensitivity, glm::radians(-89.0f), glm::radians(89.0f));
		}
		else if (m_Orbiting)
		{
			const glm::vec3 pivot = m_Position + GetForward() * m_OrbitDistance;

			m_Yaw += mouseDelta.x * m_LookSensitivity;
			m_Pitch = glm::clamp(m_Pitch - mouseDelta.y * m_LookSensitivity, glm::radians(-89.0f), glm::radians(89.0f));

			m_Position = pivot - GetForward() * m_OrbitDistance;
		}
		else if (m_Panning)
		{
			const float panSpeed = m_OrbitDistance * 0.0015f;

			m_Position += (-GetRight() * mouseDelta.x + glm::vec3(0.0f, 1.0f, 0.0f) * mouseDelta.y) * panSpeed;
		}

		if (m_Looking)
		{
			glm::vec3 movement{ 0.0f };
			const glm::vec3 forward = GetForward();
			const glm::vec3 right = GetRight();

			if (m_Input->IsKeyDown(Ignition::ScanCode::W))
			{
				movement += forward;
			}
			
			if (m_Input->IsKeyDown(Ignition::ScanCode::S))
			{
				movement -= forward;
			}
			
			if (m_Input->IsKeyDown(Ignition::ScanCode::D))
			{
				movement += right;
			}
			
			if (m_Input->IsKeyDown(Ignition::ScanCode::A))
			{
				movement -= right;
			}
			
			if (m_Input->IsKeyDown(Ignition::ScanCode::E))
			{
				movement += glm::vec3(0.0f, 1.0f, 0.0f);
			}
			
			if (m_Input->IsKeyDown(Ignition::ScanCode::Q))
			{
				movement -= glm::vec3(0.0f, 1.0f, 0.0f);
			}

			if (movement != glm::vec3(0.0f))
			{
				m_Position += glm::normalize(movement) * m_MoveSpeed * deltaTime;
			}
		}

		camera.LookAt(m_Position, m_Position + GetForward());
	}

	void EditorCameraController::Focus(const glm::vec3& point, float radius)
	{
		m_OrbitDistance = glm::max(radius * 3.0f, 1.0f);
		m_Position = point - GetForward() * m_OrbitDistance;
	}

	glm::vec3 EditorCameraController::GetForward() const
	{
		return glm::vec3(glm::cos(m_Pitch) * glm::sin(m_Yaw), glm::sin(m_Pitch), -glm::cos(m_Pitch) * glm::cos(m_Yaw));
	}

	glm::vec3 EditorCameraController::GetRight() const
	{
		const glm::vec3 forward = GetForward();

		return glm::normalize(glm::cross(glm::vec3(forward.x, 0.0f, forward.z), glm::vec3(0.0f, 1.0f, 0.0f)));
	}
}