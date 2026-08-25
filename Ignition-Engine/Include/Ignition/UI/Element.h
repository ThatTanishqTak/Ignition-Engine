#pragma once

#include "Ignition/Core/Export.h"
#include "Ignition/UI/Style.h"
#include "Ignition/UI/UITypes.h"

#include <glm/vec2.hpp>

#include <memory>
#include <string>
#include <vector>

namespace Ignition
{
	namespace UI
	{
		class DrawList;
		class UIContext;
		class UIEvent;

		class Element
		{
		public:
			IGNITION_API Element();
			IGNITION_API virtual ~Element();

			Element(const Element&) = delete;
			Element& operator=(const Element&) = delete;

			IGNITION_API Element& AddChild(std::unique_ptr<Element> child);
			IGNITION_API std::unique_ptr<Element> RemoveChild(Element* child);
			IGNITION_API void ClearChildren();

			const std::vector<std::unique_ptr<Element>>& GetChildren() const { return m_Children; }

			// Non-owning. The parent is whichever node holds this element's unique_ptr, null for a context root
			Element* GetParent() const { return m_Parent; }
			UIContext* GetContext() const { return m_Context; }

			const std::string& GetName() const { return m_Name; }
			void SetName(std::string name) { m_Name = std::move(name); }

			Style& GetStyle() { return m_Style; }
			const Style& GetStyle() const { return m_Style; }

			const Rect& GetBounds() const { return m_Bounds; }
			const glm::vec2& GetDesiredSize() const { return m_DesiredSize; }

			// Measure walks up, arrange walks down, paint stays put. Phase 3 adds the measure-boundary short-circuit
			IGNITION_API void InvalidateMeasure();
			IGNITION_API void InvalidateArrange();
			IGNITION_API void InvalidatePaint();

			DirtyFlags GetDirtyFlags() const { return m_Dirty; }

			// Keyed lookup for the Phase 5 StyleSheet and the Phase 3 tree debugger
			virtual const char* GetTypeName() const { return "Element"; }

		protected:
			virtual glm::vec2 OnMeasure(const Constraints& constraints) { return constraints.Minimum; }
			virtual void OnArrange(const Rect& bounds) { (void)bounds; }
			virtual void OnPaint(DrawList& drawList) { (void)drawList; }
			virtual void OnEvent(UIEvent& event) { (void)event; }
			virtual void OnTick(float deltaTime) { (void)deltaTime; }

		private:
			friend class UIContext;

			IGNITION_API void AttachToContext(UIContext* context);

		private:
			Element* m_Parent = nullptr;
			UIContext* m_Context = nullptr;
			std::vector<std::unique_ptr<Element>> m_Children;

			std::string m_Name;
			Style m_Style;

			Rect m_Bounds;
			Constraints m_CachedConstraints;
			glm::vec2 m_DesiredSize{ 0.0f };

			DirtyFlags m_Dirty = DirtyFlags::All;
		};
	}
}