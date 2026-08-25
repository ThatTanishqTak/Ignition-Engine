#include "Ignition/UI/Element.h"

#include "Ignition/UI/UIContext.h"

#include <algorithm>

namespace Ignition
{
	namespace UI
	{
		Element::Element() = default;

		Element::~Element()
		{
			if (m_Context)
			{
				m_Context->OnElementDestroyed(this);
			}
		}

		Element& Element::AddChild(std::unique_ptr<Element> child)
		{
			Element& reference = *child;

			child->m_Parent = this;
			child->AttachToContext(m_Context);

			m_Children.push_back(std::move(child));

			InvalidateMeasure();

			return reference;
		}

		std::unique_ptr<Element> Element::RemoveChild(Element* child)
		{
			const auto it = std::find_if(m_Children.begin(), m_Children.end(), [child](const std::unique_ptr<Element>& candidate) { return candidate.get() == child; });

			if (it == m_Children.end())
			{
				return nullptr;
			}

			std::unique_ptr<Element> removed = std::move(*it);
			m_Children.erase(it);

			removed->m_Parent = nullptr;
			removed->AttachToContext(nullptr);

			InvalidateMeasure();

			return removed;
		}

		void Element::ClearChildren()
		{
			m_Children.clear();

			InvalidateMeasure();
		}

		void Element::InvalidateMeasure()
		{
			for (Element* node = this; node != nullptr; node = node->m_Parent)
			{
				if (HasFlag(node->m_Dirty, DirtyFlags::Measure))
				{
					break;
				}

				node->m_Dirty |= DirtyFlags::All;
			}
		}

		void Element::InvalidateArrange()
		{
			m_Dirty |= DirtyFlags::Arrange | DirtyFlags::Paint;

			// Arrange propagates down from the node whose rect changed
			for (const auto& child : m_Children)
			{
				child->InvalidateArrange();
			}
		}

		void Element::InvalidatePaint()
		{
			m_Dirty |= DirtyFlags::Paint;
		}

		void Element::AttachToContext(UIContext* context)
		{
			if (m_Context == context)
			{
				return;
			}

			// Detaching hands the old context a chance to drop any pointer it holds before the element moves
			if (m_Context)
			{
				m_Context->OnElementDestroyed(this);
			}

			m_Context = context;

			for (const auto& child : m_Children)
			{
				child->AttachToContext(context);
			}
		}
	}
}