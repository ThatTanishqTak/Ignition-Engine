#include "Sandbox/GalleryLayer.h"

#include "Ignition/Ignition.h"

namespace Sandbox
{
	GalleryLayer::GalleryLayer(Ignition::UI::UIContext* context) : m_Context(context)
	{

	}

	void GalleryLayer::RegisterPage(std::string name, PageBuilder build)
	{
		m_Pages.push_back(Page{ std::move(name), std::move(build) });
	}

	void GalleryLayer::OnAttach()
	{
		if (!m_Context)
		{
			IG_APP_ERROR("Gallery: no UI context, pages will not build");

			return;
		}

		RegisterPages();

		IG_APP_INFO("Gallery: {} page(s) registered - number keys select, [ and ] step", m_Pages.size());

		ShowPage(0);
	}

	void GalleryLayer::RegisterPages()
	{
		RegisterPage("Placeholder", [](Ignition::UI::UIContext& context)
		{
			context.AddRoot(std::make_unique<Ignition::UI::Element>()).SetName("Placeholder Root");
		});
	}

	void GalleryLayer::ShowPage(size_t index)
	{
		if (!m_Context || m_Pages.empty() || index >= m_Pages.size())
		{
			return;
		}

		m_CurrentPage = index;

		m_Context->ClearRoots();
		m_Pages[m_CurrentPage].Build(*m_Context);

		IG_APP_INFO("Gallery page {}/{}: {}", m_CurrentPage + 1, m_Pages.size(), m_Pages[m_CurrentPage].Name);
	}

	void GalleryLayer::StepPage(int delta)
	{
		if (m_Pages.empty())
		{
			return;
		}

		const int count = static_cast<int>(m_Pages.size());
		const int next = (static_cast<int>(m_CurrentPage) + delta % count + count) % count;

		ShowPage(static_cast<size_t>(next));
	}

	void GalleryLayer::OnEvent(Ignition::Event& event)
	{
		Ignition::EventDispatcher dispatcher(event);

		dispatcher.Dispatch<Ignition::KeyPressedEvent>([this](Ignition::KeyPressedEvent& keyEvent)
		{
			if (keyEvent.IsRepeat())
			{
				return false;
			}

			const Ignition::KeyCode key = keyEvent.GetKeyCode();

			if (key >= Ignition::KeyCode::D1 && key <= Ignition::KeyCode::D9)
			{
				const auto index = static_cast<size_t>(static_cast<uint32_t>(key) - static_cast<uint32_t>(Ignition::KeyCode::D1));

				if (index < m_Pages.size())
				{
					ShowPage(index);
				}

				return false;
			}

			if (key == Ignition::KeyCode::LEFTBRACKET)
			{
				StepPage(-1);
			}
			else if (key == Ignition::KeyCode::RIGHTBRACKET)
			{
				StepPage(1);
			}

			return false;
		});
	}
}