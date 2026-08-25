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

			(void)deltaTime;

			m_LayoutPassCount = 0;
		}

		void UIContext::OnElementDestroyed(Element* element)
		{
			(void)element;
		}
	}
}