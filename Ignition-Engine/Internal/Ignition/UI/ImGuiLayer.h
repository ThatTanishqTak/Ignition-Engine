#pragma once

#include "Ignition/Core/Layer.h"

namespace Ignition
{
	class Renderer;
	class Input;

	class ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer(Renderer* renderer, Input* input);

		void BeginFrame();

		void OnRender() override;
		void OnEvent(Event& event) override;

	private:
		Renderer* m_Renderer = nullptr;
		Input* m_Input = nullptr;
	};
}