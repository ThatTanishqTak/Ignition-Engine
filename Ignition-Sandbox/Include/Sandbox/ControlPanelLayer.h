#pragma once

#include <Ignition/Core/Layer.h>

#include <cstdint>
#include <vector>

namespace Ignition
{
	class Renderer;
	class Scene;
}

namespace Sandbox
{
	class ControlPanelLayer final : public Ignition::Layer
	{
	public:
		ControlPanelLayer(Ignition::Renderer* renderer, Ignition::Scene* scene);

		void OnUpdate(float deltaTime) override;
		void OnRender() override;

	private:
		void DrawSessionSection();
		void DrawSceneSection();
		void SpawnEntity();

	private:
		Ignition::Renderer* m_Renderer = nullptr;
		Ignition::Scene* m_Scene = nullptr;

		std::vector<float> m_FrameTimeHistory;
		uint32_t m_SpawnCount = 0;
	};
}