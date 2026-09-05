#include "Ignition/UI/UIContext.h"

#include "Ignition/Core/Log.h"
#include "Ignition/Core/ProfilerInternal.h"

#include <algorithm>

namespace Ignition
{
	namespace UI
	{
		UIContext::UIContext()
		{
			IG_CORE_INFO("UI context created");
		}

		UIContext::~UIContext()
		{
			// Roots drop first so every ~Element still sees a live context to report into
			ClearRoots();

			IG_CORE_INFO("UI context destroyed");
		}

		Element& UIContext::AddRoot(std::unique_ptr<Element> root)
		{
			Element& reference = *root;

			root->AttachToContext(this);
			m_Roots.push_back(std::move(root));

			return reference;
		}

		void UIContext::ClearRoots()
		{
			m_Roots.clear();
		}

		void UIContext::SetSurfaceSize(const glm::vec2& size)
		{
			if (m_SurfaceSize == size)
			{
				return;
			}

			m_SurfaceSize = size;

			for (const auto& root : m_Roots)
			{
				root->InvalidateMeasure();
			}
		}

		void UIContext::Tick(float deltaTime)
		{
			IG_PROFILE_ZONE_NAMED("UI Tick");

			m_LayoutPassCount = 0;

			for (const auto& root : m_Roots)
			{
				TickElement(*root, deltaTime);
			}
		}

		void UIContext::TickElement(Element& element, float deltaTime)
		{
			element.OnTick(deltaTime);

			for (const auto& child : element.GetChildren())
			{
				TickElement(*child, deltaTime);
			}
		}

		void UIContext::Paint(DrawList& drawList)
		{
			IG_PROFILE_ZONE_NAMED("UI Paint");

			for (const auto& root : m_Roots)
			{
				PaintElement(*root, drawList);
			}
		}

		void UIContext::PaintElement(Element& element, DrawList& drawList)
		{
			element.OnPaint(drawList);

			for (const auto& child : element.GetChildren())
			{
				PaintElement(*child, drawList);
			}
		}

		void UIContext::OnElementDestroyed(Element* element)
		{
			(void)element;
		}
	}
}