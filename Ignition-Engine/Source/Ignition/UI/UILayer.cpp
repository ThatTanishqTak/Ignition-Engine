#include "Ignition/UI/UILayer.h"

#include "Ignition/Core/ProfilerInternal.h"
#include "Ignition/Core/Time.h"
#include "Ignition/Events/Event.h"
#include "Ignition/Events/WindowEvent.h"
#include "Ignition/Renderer/Vulkan/VulkanRenderer.h"
#include "Ignition/UI/UIContext.h"
#include "Ignition/Window/Window.h"

namespace Ignition
{
	UILayer::UILayer(Window* window, VulkanRenderer* backend) : m_Window(window), m_Backend(backend)
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
		IG_PROFILE_ZONE_NAMED("UI Build");

		if (!m_Context || !m_Backend)
		{
			return;
		}

		m_DrawList.Clear(m_Context->GetSurfaceSize());
		m_Context->Paint(m_DrawList);
		m_DrawList.Finish();

		// The layer is an overlay, so this runs after every content layer has rendered and before EndFrame records
		m_Backend->SubmitUI(m_DrawList, UI::UISurfaceTarget::Swapchain);
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