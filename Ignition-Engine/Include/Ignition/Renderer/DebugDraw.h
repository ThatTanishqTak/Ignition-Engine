#pragma once

#include "Ignition/Core/Export.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace Ignition
{
	// Immediate-mode line drawing: submit any time during a frame, the renderer flushes it into the scene pass
	class DebugDraw
	{
	public:
		IGNITION_API static void SetEnabled(bool enabled);
		IGNITION_API static bool IsEnabled();

		IGNITION_API static void Line(const glm::vec3& from, const glm::vec3& to, const glm::vec4& color = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), bool depthTested = true);
		IGNITION_API static void Arrow(const glm::vec3& from, const glm::vec3& to, const glm::vec4& color = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f), bool depthTested = true);

		IGNITION_API static void Box(const glm::mat4& transform, const glm::vec3& halfExtents, const glm::vec4& color = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), bool depthTested = true);
		IGNITION_API static void Sphere(const glm::mat4& transform, float radius, const glm::vec4& color = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), bool depthTested = true);
		IGNITION_API static void Capsule(const glm::mat4& transform, float radius, float halfHeight, const glm::vec4& color = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), bool depthTested = true);

		IGNITION_API static void Axes(const glm::mat4& transform, float length = 1.0f, bool depthTested = false);
		IGNITION_API static void Grid(const glm::vec3& center, float extent, float spacing, const glm::vec4& color = glm::vec4(0.25f, 0.25f, 0.25f, 1.0f));
	};
}