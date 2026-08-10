#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/trigonometric.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Ignition
{
	class Camera
	{
	public:
		Camera()
		{
			RecalculateProjection();
		}

		void SetPerspective(float verticalFOV, float aspectRatio, float nearClip, float farClip)
		{
			m_VerticalFOV = verticalFOV;
			m_AspectRatio = aspectRatio;
			m_NearClip = nearClip;
			m_FarClip = farClip;

			RecalculateProjection();
		}

		void SetAspectRatio(float aspectRatio)
		{
			m_AspectRatio = aspectRatio;

			RecalculateProjection();
		}

		void LookAt(const glm::vec3& eye, const glm::vec3& target, const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f))
		{
			m_View = glm::lookAt(eye, target, up);
		}

		const glm::mat4& GetView() const { return m_View; }
		const glm::mat4& GetProjection() const { return m_Projection; }
		glm::mat4 GetViewProjection() const { return m_Projection * m_View; }

	private:
		void RecalculateProjection()
		{
			m_Projection = glm::perspective(m_VerticalFOV, m_AspectRatio, m_NearClip, m_FarClip);
		}

		glm::mat4 m_View{ 1.0f };
		glm::mat4 m_Projection{ 1.0f };

		float m_VerticalFOV = glm::radians(60.0f);
		float m_AspectRatio = 16.0f / 9.0f;
		float m_NearClip = 0.1f;
		float m_FarClip = 100.0f;
	};
}