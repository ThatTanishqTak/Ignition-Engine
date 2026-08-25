#pragma once

#include <Ignition/Core/Layer.h>

#include <functional>
#include <string>
#include <vector>

namespace Ignition
{
	namespace UI
	{
		class UIContext;
	}
}

namespace Sandbox
{
	class GalleryLayer final : public Ignition::Layer
	{
	public:
		using PageBuilder = std::function<void(Ignition::UI::UIContext&)>;

		explicit GalleryLayer(Ignition::UI::UIContext* context);

		void RegisterPage(std::string name, PageBuilder build);

		void OnAttach() override;
		void OnEvent(Ignition::Event& event) override;

	private:
		void RegisterPages();
		void ShowPage(size_t index);
		void StepPage(int delta);

	private:
		struct Page
		{
			std::string Name;
			PageBuilder Build;
		};

		Ignition::UI::UIContext* m_Context = nullptr;
		std::vector<Page> m_Pages;
		size_t m_CurrentPage = 0;
	};
}