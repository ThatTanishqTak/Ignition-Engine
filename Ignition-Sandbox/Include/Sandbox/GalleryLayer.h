#pragma once

#include <Ignition/Core/Layer.h>

#include <functional>
#include <string>
#include <vector>

#include <glm/vec2.hpp>

namespace Ignition
{
	namespace UI
	{
		class UIContext;
	}
}

namespace Sandbox
{
	class NavStripElement;

	class GalleryLayer final : public Ignition::Layer
	{
	public:
		using PageBuilder = std::function<void(Ignition::UI::UIContext&)>;

		explicit GalleryLayer(Ignition::UI::UIContext* context);

		void RegisterPage(std::string name, PageBuilder build);

		static glm::vec2 ToSurface(float x, float y) { return glm::vec2(x, y); }
		const glm::vec2& GetPointer() const { return m_Pointer; }

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

		// Non-owning: the context owns the chrome root, this is the handle the hand hit test reads its rects from
		NavStripElement* m_NavStrip = nullptr;

		// A button event carries its modifiers but not its position, so the last move is the only pointer state there is
		glm::vec2 m_Pointer{ -1.0f };
	};
}