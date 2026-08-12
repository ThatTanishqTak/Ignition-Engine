#pragma once

namespace Ignition
{
	class Event;

	class Layer
	{
	public:
		Layer() = default;
		virtual ~Layer() = default;

		Layer(const Layer&) = delete;
		Layer& operator=(const Layer&) = delete;

		virtual void OnAttach() {}
		virtual void OnDetach() {}
		virtual void OnUpdate(float deltaTime) { (void)deltaTime; }
		virtual void OnFixedUpdate(float fixedTimeStep) { (void)fixedTimeStep; }
		virtual void OnRender() {}
		virtual void OnEvent(Event& event) { (void)event; }
	};
}