#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <vector>

namespace Ignition
{
	struct DebugLineVertex
	{
		glm::vec3 Position;
		glm::vec4 Color;
	};

	class DebugDrawBuffer
	{
	public:
		static DebugDrawBuffer& Get();

		void Add(const glm::vec3& from, const glm::vec3& to, const glm::vec4& color, bool depthTested);
		void Clear();

		const std::vector<DebugLineVertex>& GetDepthTestedVertices() const { return m_DepthTested; }
		const std::vector<DebugLineVertex>& GetOverlayVertices() const { return m_Overlay; }

		void SetEnabled(bool enabled) { m_Enabled = enabled; }
		bool IsEnabled() const { return m_Enabled; }

	private:
		std::vector<DebugLineVertex> m_DepthTested;
		std::vector<DebugLineVertex> m_Overlay;

		bool m_Enabled = true;
	};
}