#pragma once

#include "Ignition/Core/Layer.h"
#include "Ignition/UI/DrawList.h"

#include <memory>

namespace Ignition
{
	class Window;
	class VulkanRenderer;

	namespace UI
	{
		class UIContext;
	}

	class UILayer : public Layer
	{
	public:
		UILayer(Window* window, VulkanRenderer* backend);
		~UILayer() override;

		UI::UIContext* GetContext() const { return m_Context.get(); }

		void OnAttach() override;
		void OnDetach() override;
		void OnUpdate(float deltaTime) override;
		void OnRender() override;
		void OnEvent(Event& event) override;

	private:
		void SyncSurfaceSize();

	private:
		Window* m_Window = nullptr;
		VulkanRenderer* m_Backend = nullptr;
		std::unique_ptr<UI::UIContext> m_Context;

		// One list, rebuilt every frame and kept alive until the backend has recorded it
		UI::DrawList m_DrawList;
	};
}