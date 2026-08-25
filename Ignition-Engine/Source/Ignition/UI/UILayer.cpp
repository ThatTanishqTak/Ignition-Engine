#include "Ignition/UI/UILayer.h"

#include "Ignition/Core/Time.h"
#include "Ignition/Events/Event.h"
#include "Ignition/Events/WindowEvent.h"
#include "Ignition/UI/UIContext.h"
#include "Ignition/Window/Window.h"

namespace Ignition
{
	UILayer::UILayer(Window* window) : m_Window(window)
	{

	}

	UILayer::~UILayer() = default;

	void UILayer::OnAttach()
	{
		m_Context = std::make_unique<UI::UIContext>();

		SyncSurfaceSize();
	}

	void UILayer::OnDetach()
	{
		m_Context.reset();
	}

	void UILayer::OnUpdate(float deltaTime)
	{
		(void)deltaTime;

		if (m_Context)
		{
			// Unscaled, deliberately: the UI keeps running at wall-clock speed while the simulation is time-scaled
			m_Context->Tick(Time::GetUnscaledDeltaTime());
		}
	}

	void UILayer::OnRender()
	{

	}

	void UILayer::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);

		dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent&)
		{
			SyncSurfaceSize();

			return false;
		});
	}

	void UILayer::SyncSurfaceSize()
	{
		if (!m_Context || !m_Window)
		{
			return;
		}

		int pixelWidth = 0;
		int pixelHeight = 0;
		m_Window->GetPixelSize(pixelWidth, pixelHeight);

		m_Context->SetSurfaceSize(glm::vec2(static_cast<float>(pixelWidth), static_cast<float>(pixelHeight)));
	}
}