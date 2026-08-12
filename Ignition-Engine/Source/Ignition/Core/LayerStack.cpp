#include "Ignition/Core/LayerStack.h"

#include "Ignition/Events/Event.h"

namespace Ignition
{
	LayerStack::~LayerStack()
	{
		Clear();
	}

	void LayerStack::PushLayer(std::unique_ptr<Layer> layer)
	{
		Layer* raw = layer.get();

		m_Layers.insert(m_Layers.begin() + m_LayerInsertIndex, std::move(layer));
		++m_LayerInsertIndex;

		raw->OnAttach();
	}

	void LayerStack::PushOverlay(std::unique_ptr<Layer> overlay)
	{
		Layer* raw = overlay.get();

		m_Layers.push_back(std::move(overlay));

		raw->OnAttach();
	}

	void LayerStack::Clear()
	{
		for (auto it = m_Layers.rbegin(); it != m_Layers.rend(); ++it)
		{
			(*it)->OnDetach();
		}

		m_Layers.clear();
		m_LayerInsertIndex = 0;
	}

	void LayerStack::OnUpdate(float deltaTime)
	{
		for (const auto& layer : m_Layers)
		{
			layer->OnUpdate(deltaTime);
		}
	}

	void LayerStack::OnFixedUpdate(float fixedTimeStep)
	{
		for (const auto& layer : m_Layers)
		{
			layer->OnFixedUpdate(fixedTimeStep);
		}
	}

	void LayerStack::OnRender()
	{
		for (const auto& layer : m_Layers)
		{
			layer->OnRender();
		}
	}

	void LayerStack::OnEvent(Event& event)
	{
		for (auto it = m_Layers.rbegin(); it != m_Layers.rend(); ++it)
		{
			(*it)->OnEvent(event);

			if (event.IsHandled())
			{
				break;
			}
		}
	}
}