#include "Ignition/UI/ImGuiLayer.h"

#include "Ignition/Core/Time.h"
#include "Ignition/Events/Event.h"
#include "Ignition/Input/Input.h"
#include "Ignition/Renderer/Renderer.h"

#include <imgui.h>

namespace Ignition
{
	ImGuiLayer::ImGuiLayer(Renderer* renderer, Input* input) : m_Renderer(renderer), m_Input(input)
	{

	}

	void ImGuiLayer::BeginFrame()
	{
		if (m_Renderer && m_Renderer->IsImGuiFrameActive())
		{
			ImGui::DockSpaceOverViewport(0, nullptr, ImGuiDockNodeFlags_PassthruCentralNode);
		}
	}

	void ImGuiLayer::OnRender()
	{
		if (m_Renderer && m_Renderer->IsImGuiFrameActive())
		{

		}
	}

	void ImGuiLayer::OnEvent(Event& event)
	{
		if (!m_Renderer)
		{
			return;
		}

		if (m_Renderer->WantCaptureMouse() && (event.IsInCategory(EventCategoryMouse) || event.IsInCategory(EventCategoryMouseButton)))
		{
			event.SetHandled(true);
		}

		if (m_Renderer->WantCaptureKeyboard() && event.IsInCategory(EventCategoryKeyboard))
		{
			event.SetHandled(true);
		}
	}
}