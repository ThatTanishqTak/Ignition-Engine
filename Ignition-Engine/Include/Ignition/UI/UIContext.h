#pragma once

#include "Ignition/Core/Export.h"
#include "Ignition/UI/Element.h"

#include <glm/vec2.hpp>

#include <memory>
#include <vector>

namespace Ignition
{
	namespace UI
	{
		class DrawList;

		class UIContext
		{
		public:
			IGNITION_API UIContext();
			IGNITION_API ~UIContext();

			UIContext(const UIContext&) = delete;
			UIContext& operator=(const UIContext&) = delete;

			IGNITION_API Element& AddRoot(std::unique_ptr<Element> root);
			IGNITION_API void ClearRoots();

			const std::vector<std::unique_ptr<Element>>& GetRoots() const { return m_Roots; }

			IGNITION_API void SetSurfaceSize(const glm::vec2& size);
			const glm::vec2& GetSurfaceSize() const { return m_SurfaceSize; }

			IGNITION_API void Tick(float deltaTime);

			IGNITION_API void Paint(DrawList& drawList);

			IGNITION_API void OnElementDestroyed(Element* element);

		private:
			void PaintElement(Element& element, DrawList& drawList);
			void TickElement(Element& element, float deltaTime);

		private:
			std::vector<std::unique_ptr<Element>> m_Roots;
			glm::vec2 m_SurfaceSize{ 0.0f };

			uint32_t m_LayoutPassCount = 0;
		};
	}
}