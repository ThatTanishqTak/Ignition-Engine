#include "Sandbox/ControlPanelLayer.h"

#include "Ignition/Ignition.h"

namespace Sandbox
{
	namespace
	{
		constexpr size_t FrameHistorySize = 240;
	}

	ControlPanelLayer::ControlPanelLayer(Ignition::Scene* scene) : m_Scene(scene)
	{
		m_FrameTimeHistory.reserve(FrameHistorySize);
	}

	void ControlPanelLayer::OnUpdate(float deltaTime)
	{
		(void)deltaTime;

		if (m_FrameTimeHistory.size() >= FrameHistorySize)
		{
			m_FrameTimeHistory.erase(m_FrameTimeHistory.begin());
		}

		m_FrameTimeHistory.push_back(Ignition::Time::GetUnscaledDeltaTime().GetMilliseconds());
	}

	void ControlPanelLayer::OnRender()
	{
		Ignition::UI::SetNextWindowSize(360.0f, 240.0f);

		if (Ignition::UI::BeginWindow("Control Panel"))
		{
			DrawSessionSection();
		}

		Ignition::UI::EndWindow();
	}

	void ControlPanelLayer::DrawSessionSection()
	{
		if (!Ignition::UI::CollapsingHeader("Session"))
		{
			return;
		}

		Ignition::UI::Text("FPS: {:.1f}", Ignition::Time::GetFramesPerSecond());
		Ignition::UI::Text("Frame Time: {:.2f} ms", Ignition::Time::GetAverageFrameTimeMilliseconds());
		Ignition::UI::Text("Entities: {}", m_Scene->GetEntities().size());

		if (!m_FrameTimeHistory.empty())
		{
			Ignition::UI::PlotLines("##FrameTimes", m_FrameTimeHistory.data(), static_cast<int>(m_FrameTimeHistory.size()), 0.0f, 33.3f, 60.0f, "Frame time (ms)");
		}

		Ignition::UI::Spacing();
	}
}