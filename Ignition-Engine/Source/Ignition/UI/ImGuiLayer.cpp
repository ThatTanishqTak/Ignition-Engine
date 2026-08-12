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
			DrawDebugWindows();
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

	void ImGuiLayer::DrawDebugWindows()
	{
		ImGui::Begin("Stats");
		ImGui::Text("FPS: %.1f", Time::GetFramesPerSecond());
		ImGui::Text("Frame Time: %.2f ms", Time::GetAverageFrameTimeMilliseconds());
		ImGui::End();

		ImGui::Begin("Gamepads");

		const auto gamepads = m_Input->GetConnectedGamepads();

		if (gamepads.empty())
		{
			ImGui::TextUnformatted("No gamepads connected");
		}

		for (const GamepadID gamepadID : gamepads)
		{
			ImGui::Text("[%d] %s", static_cast<int>(gamepadID), m_Input->GetGamepadName(gamepadID).c_str());
		}

		ImGui::End();
	}
}