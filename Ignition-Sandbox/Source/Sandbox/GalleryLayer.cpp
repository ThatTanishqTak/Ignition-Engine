#include "Sandbox/GalleryLayer.h"

#include "Sandbox/GalleryPages.h"

#include "Ignition/Ignition.h"

namespace Sandbox
{
	GalleryLayer::GalleryLayer(Ignition::UI::UIContext* context, Ignition::Renderer* renderer) : m_Context(context), m_Renderer(renderer)
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
		RegisterPage("Primitives", [](Ignition::UI::UIContext& context)
		{
			context.AddRoot(std::make_unique<PrimitivesPage>()).SetName("Primitives");
		});

		RegisterPage("Clipping and layers", [](Ignition::UI::UIContext& context)
		{
			context.AddRoot(std::make_unique<ClippingPage>()).SetName("Clipping");
		});

		RegisterPage("Tessellator stress", [](Ignition::UI::UIContext& context)
		{
			context.AddRoot(std::make_unique<TessellatorPage>()).SetName("Tessellator");
		});

		RegisterPage("Scene target", [this](Ignition::UI::UIContext& context)
		{
			context.AddRoot(std::make_unique<SceneTargetPage>(m_Renderer));
		});

		RegisterPage("Primitive stress", [](Ignition::UI::UIContext& context)
		{
			context.AddRoot(std::make_unique<StressPage>()).SetName("Stress");
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
		m_NavStrip = nullptr;

		m_Pages[m_CurrentPage].Build(*m_Context);

		std::vector<std::string> names;
		names.reserve(m_Pages.size());

		for (const Page& page : m_Pages)
		{
			names.push_back(page.Name);
		}

		auto navStrip = std::make_unique<NavStripElement>(std::move(names), [this](size_t selected) { ShowPage(selected); });
		m_NavStrip = navStrip.get();
		m_NavStrip->SetActive(m_CurrentPage);
		m_NavStrip->SetPointer(m_Pointer);

		m_Context->AddRoot(std::move(navStrip));

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

		dispatcher.Dispatch<Ignition::MouseMovedEvent>([this](Ignition::MouseMovedEvent& mouseEvent)
		{
			m_Pointer = ToSurface(mouseEvent.GetX(), mouseEvent.GetY());

			if (m_NavStrip)
			{
				m_NavStrip->SetPointer(m_Pointer);
			}

			return false;
		});

		dispatcher.Dispatch<Ignition::MouseButtonPressedEvent>([this](Ignition::MouseButtonPressedEvent& mouseEvent)
		{
			if (m_NavStrip && mouseEvent.GetMouseButton() == Ignition::MouseCode::LEFT)
			{
				// ShowPage rebuilds the strip, so nothing may touch m_NavStrip after this returns
				m_NavStrip->OnPointerDown(m_Pointer);
			}

			return false;
		});

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
			else if (key == Ignition::KeyCode::S && m_Context)
			{
				const Ignition::UI::DrawStatistics statistics = m_Context->GetDrawStatistics();

				IG_APP_INFO("UI frame: {} draw call(s), {} vertices, {} indices, {} primitives", statistics.DrawCalls, statistics.Vertices, statistics.Indices, statistics.Primitives);
			}

			return false;
		});
	}
}