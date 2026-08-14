#pragma once

#include <Ignition/Core/Layer.h>

#include <vector>

namespace Ignition
{
	class Scene;
}

namespace Sandbox
{
	class ControlPanelLayer final : public Ignition::Layer
	{
	public:
		explicit ControlPanelLayer(Ignition::Scene* scene);

		void OnUpdate(float deltaTime) override;
		void OnRender() override;

	private:
		void DrawSessionSection();

	private:
		Ignition::Scene* m_Scene = nullptr;

		std::vector<float> m_FrameTimeHistory;
	};
}