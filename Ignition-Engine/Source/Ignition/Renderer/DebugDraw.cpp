#include "Ignition/Renderer/DebugDraw.h"

#include "Ignition/Renderer/DebugDrawBuffer.h"

#include <glm/gtc/constants.hpp>
#include <glm/geometric.hpp>

#include <array>
#include <cmath>

namespace Ignition
{
	DebugDrawBuffer& DebugDrawBuffer::Get()
	{
		static DebugDrawBuffer buffer;

		return buffer;
	}

	void DebugDrawBuffer::Add(const glm::vec3& from, const glm::vec3& to, const glm::vec4& color, bool depthTested)
	{
		if (!m_Enabled)
		{
			return;
		}

		std::vector<DebugLineVertex>& target = depthTested ? m_DepthTested : m_Overlay;

		target.push_back({ from, color });
		target.push_back({ to, color });
	}

	void DebugDrawBuffer::Clear()
	{
		m_DepthTested.clear();
		m_Overlay.clear();
	}

	namespace
	{
		glm::vec3 TransformPoint(const glm::mat4& transform, const glm::vec3& point)
		{
			return glm::vec3(transform * glm::vec4(point, 1.0f));
		}

		void DrawCircle(const glm::mat4& transform, const glm::vec3& center, const glm::vec3& axisU, const glm::vec3& axisV, float radius, const glm::vec4& color, bool depthTested, int segments = 24)
		{
			glm::vec3 previous = TransformPoint(transform, center + axisU * radius);

			for (int segment = 1; segment <= segments; ++segment)
			{
				const float angle = glm::two_pi<float>() * static_cast<float>(segment) / static_cast<float>(segments);
				const glm::vec3 current = TransformPoint(transform, center + (axisU * std::cos(angle) + axisV * std::sin(angle)) * radius);

				DebugDrawBuffer::Get().Add(previous, current, color, depthTested);
				previous = current;
			}
		}
	}

	void DebugDraw::SetEnabled(bool enabled)
	{
		DebugDrawBuffer::Get().SetEnabled(enabled);
	}

	bool DebugDraw::IsEnabled()
	{
		return DebugDrawBuffer::Get().IsEnabled();
	}

	void DebugDraw::Line(const glm::vec3& from, const glm::vec3& to, const glm::vec4& color, bool depthTested)
	{
		DebugDrawBuffer::Get().Add(from, to, color, depthTested);
	}

	void DebugDraw::Arrow(const glm::vec3& from, const glm::vec3& to, const glm::vec4& color, bool depthTested)
	{
		DebugDrawBuffer::Get().Add(from, to, color, depthTested);

		const glm::vec3 shaft = to - from;
		const float length = glm::length(shaft);

		if (length <= 0.0001f)
		{
			return;
		}

		const glm::vec3 direction = shaft / length;
		const glm::vec3 reference = std::abs(direction.y) > 0.9f ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
		const glm::vec3 right = glm::normalize(glm::cross(direction, reference));
		const glm::vec3 up = glm::cross(right, direction);

		const float headLength = length * 0.15f;
		const float headRadius = headLength * 0.5f;
		const glm::vec3 headBase = to - direction * headLength;

		DebugDrawBuffer::Get().Add(to, headBase + right * headRadius, color, depthTested);
		DebugDrawBuffer::Get().Add(to, headBase - right * headRadius, color, depthTested);
		DebugDrawBuffer::Get().Add(to, headBase + up * headRadius, color, depthTested);
		DebugDrawBuffer::Get().Add(to, headBase - up * headRadius, color, depthTested);
	}

	void DebugDraw::Box(const glm::mat4& transform, const glm::vec3& halfExtents, const glm::vec4& color, bool depthTested)
	{
		std::array<glm::vec3, 8> corners{};

		for (int corner = 0; corner < 8; ++corner)
		{
			const glm::vec3 sign(
				(corner & 1) ? 1.0f : -1.0f,
				(corner & 2) ? 1.0f : -1.0f,
				(corner & 4) ? 1.0f : -1.0f);

			corners[corner] = TransformPoint(transform, halfExtents * sign);
		}

		constexpr std::array<std::pair<int, int>, 12> edges = { {
			{ 0, 1 }, { 1, 3 }, { 3, 2 }, { 2, 0 },
			{ 4, 5 }, { 5, 7 }, { 7, 6 }, { 6, 4 },
			{ 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 },
		} };

		for (const auto& [first, second] : edges)
		{
			DebugDrawBuffer::Get().Add(corners[first], corners[second], color, depthTested);
		}
	}

	void DebugDraw::Sphere(const glm::mat4& transform, float radius, const glm::vec4& color, bool depthTested)
	{
		DrawCircle(transform, glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), radius, color, depthTested);
		DrawCircle(transform, glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), radius, color, depthTested);
		DrawCircle(transform, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), radius, color, depthTested);
	}

	void DebugDraw::Capsule(const glm::mat4& transform, float radius, float halfHeight, const glm::vec4& color, bool depthTested)
	{
		const glm::vec3 top(0.0f, halfHeight, 0.0f);
		const glm::vec3 bottom(0.0f, -halfHeight, 0.0f);

		DrawCircle(transform, top, glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), radius, color, depthTested);
		DrawCircle(transform, bottom, glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), radius, color, depthTested);

		DrawCircle(transform, top, glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), radius, color, depthTested, 12);
		DrawCircle(transform, bottom, glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), radius, color, depthTested, 12);

		for (int side = 0; side < 4; ++side)
		{
			const float angle = glm::half_pi<float>() * static_cast<float>(side);
			const glm::vec3 offset(std::cos(angle) * radius, 0.0f, std::sin(angle) * radius);

			DebugDrawBuffer::Get().Add(TransformPoint(transform, top + offset), TransformPoint(transform, bottom + offset), color, depthTested);
		}
	}

	void DebugDraw::Axes(const glm::mat4& transform, float length, bool depthTested)
	{
		const glm::vec3 origin = TransformPoint(transform, glm::vec3(0.0f));

		DebugDrawBuffer::Get().Add(origin, TransformPoint(transform, glm::vec3(length, 0.0f, 0.0f)), glm::vec4(1.0f, 0.2f, 0.2f, 1.0f), depthTested);
		DebugDrawBuffer::Get().Add(origin, TransformPoint(transform, glm::vec3(0.0f, length, 0.0f)), glm::vec4(0.2f, 1.0f, 0.2f, 1.0f), depthTested);
		DebugDrawBuffer::Get().Add(origin, TransformPoint(transform, glm::vec3(0.0f, 0.0f, length)), glm::vec4(0.2f, 0.4f, 1.0f, 1.0f), depthTested);
	}

	void DebugDraw::Grid(const glm::vec3& center, float extent, float spacing, const glm::vec4& color)
	{
		if (spacing <= 0.0f)
		{
			return;
		}

		for (float offset = -extent; offset <= extent + 0.0001f; offset += spacing)
		{
			DebugDrawBuffer::Get().Add(center + glm::vec3(offset, 0.0f, -extent), center + glm::vec3(offset, 0.0f, extent), color, true);
			DebugDrawBuffer::Get().Add(center + glm::vec3(-extent, 0.0f, offset), center + glm::vec3(extent, 0.0f, offset), color, true);
		}
	}
}