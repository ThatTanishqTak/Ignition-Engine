#pragma once

#include "Ignition/Core/Layer.h"

#include <memory>
#include <vector>

namespace Ignition
{
	class Event;

	class LayerStack
	{
	public:
		LayerStack() = default;
		~LayerStack();

		LayerStack(const LayerStack&) = delete;
		LayerStack& operator=(const LayerStack&) = delete;

		void PushLayer(std::unique_ptr<Layer> layer);
		void PushOverlay(std::unique_ptr<Layer> overlay);

		void Clear();

		void OnUpdate(float deltaTime);
		void OnFixedUpdate(float fixedTimeStep);
		void OnRender();
		void OnEvent(Event& event);

	private:
		std::vector<std::unique_ptr<Layer>> m_Layers;
		size_t m_LayerInsertIndex = 0;
	};
}