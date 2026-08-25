#pragma once

#include "Ignition/Core/Layer.h"

#include <memory>

namespace Ignition
{
	class Window;

	namespace UI
	{
		class UIContext;
	}

	class UILayer : public Layer
	{
	public:
		explicit UILayer(Window* window);
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
		std::unique_ptr<UI::UIContext> m_Context;
	};
}