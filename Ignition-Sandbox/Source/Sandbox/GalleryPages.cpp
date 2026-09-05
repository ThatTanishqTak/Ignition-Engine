#include "Sandbox/GalleryPages.h"

#include <Ignition/Core/Log.h>
#include <Ignition/Renderer/Renderer.h>
#include <Ignition/UI/Tessellator.h>
#include <Ignition/UI/UIContext.h>

#include <algorithm>
#include <cmath>

namespace Sandbox
{
	namespace
	{
		using Ignition::UI::CornerRadii;
		using Ignition::UI::DrawLayer;
		using Ignition::UI::Rect;

		// Linear, because that is what the shader and the swapchain both expect. Phase 5 makes these theme tokens
		constexpr glm::vec4 Surface{ 0.055f, 0.059f, 0.070f, 1.0f };
		constexpr glm::vec4 Raised{ 0.100f, 0.106f, 0.125f, 1.0f };
		constexpr glm::vec4 Accent{ 0.145f, 0.400f, 0.850f, 1.0f };
		constexpr glm::vec4 Outline{ 0.320f, 0.350f, 0.420f, 1.0f };
		constexpr glm::vec4 Ink{ 0.850f, 0.870f, 0.900f, 1.0f };

		float Pulse(float phase, float low, float high)
		{
			return low + (high - low) * (0.5f + 0.5f * std::sin(phase));
		}
	}

	NavStripElement::NavStripElement(std::vector<std::string> names, std::function<void(size_t)> onSelect) : m_Names(std::move(names)), m_Select(std::move(onSelect))
	{
		SetName("Nav Strip");
	}

	Rect NavStripElement::GetTabRect(size_t index) const
	{
		return Rect{ glm::vec2(8.0f + static_cast<float>(index) * (TabWidth + 4.0f), 5.0f), glm::vec2(TabWidth, Height - 10.0f) };
	}

	bool NavStripElement::OnPointerDown(const glm::vec2& position)
	{
		for (size_t index = 0; index < m_Names.size(); ++index)
		{
			if (GetTabRect(index).Contains(position))
			{
				if (m_Select)
				{
					m_Select(index);
				}

				return true;
			}
		}

		return false;
	}

	void NavStripElement::OnPaint(Ignition::UI::DrawList& drawList)
	{
		const glm::vec2 surface = GetContext() ? GetContext()->GetSurfaceSize() : glm::vec2(0.0f);

		// Chrome sits above content whatever order the pages painted in
		drawList.PushLayer(DrawLayer::Chrome);

		drawList.AddShadow(Rect{ glm::vec2(0.0f), glm::vec2(surface.x, Height) }, glm::vec4(0.0f, 0.0f, 0.0f, 0.55f), 12.0f, {}, glm::vec2(0.0f, 2.0f));
		drawList.AddRect(Rect{ glm::vec2(0.0f), glm::vec2(surface.x, Height) }, Raised);

		for (size_t index = 0; index < m_Names.size(); ++index)
		{
			const Rect tab = GetTabRect(index);
			const bool active = index == m_Active;
			const bool hovered = tab.Contains(m_Pointer);

			drawList.AddRect(tab, active ? Accent : (hovered ? glm::vec4(0.16f, 0.17f, 0.20f, 1.0f) : glm::vec4(0.0f)), CornerRadii::Uniform(5.0f));

			if (!active)
			{
				drawList.AddRectBorder(tab, Outline, 1.0f, CornerRadii::Uniform(5.0f));
			}

			for (size_t bar = 0; bar <= index; ++bar)
			{
				drawList.AddRect(Rect{ glm::vec2(tab.GetLeft() + 12.0f + static_cast<float>(bar) * 8.0f, tab.GetTop() + 7.0f), glm::vec2(4.0f, tab.Size.y - 14.0f) }, active ? Ink : Outline, CornerRadii::Uniform(2.0f));
			}
		}

		drawList.PopLayer();
	}

	void PrimitivesPage::OnPaint(Ignition::UI::DrawList& drawList)
	{
		const glm::vec2 surface = GetContext() ? GetContext()->GetSurfaceSize() : glm::vec2(0.0f);

		drawList.AddRect(Rect{ glm::vec2(0.0f), surface }, Surface);

		for (int index = 0; index < 8; ++index)
		{
			const float radius = Pulse(m_Phase + static_cast<float>(index) * 0.4f, 0.0f, 36.0f);
			const Rect box{ glm::vec2(48.0f + static_cast<float>(index) * 104.0f, 90.0f), glm::vec2(88.0f) };

			drawList.AddShadow(box, glm::vec4(0.0f, 0.0f, 0.0f, 0.5f), 18.0f, CornerRadii::Uniform(radius), glm::vec2(0.0f, 6.0f));
			drawList.AddRect(box, Accent, CornerRadii::Uniform(radius));
			drawList.AddRectBorder(box, Ink, 2.0f, CornerRadii::Uniform(radius));
		}

		// Per-corner radii, which is the case a single-radius shader silently gets wrong
		drawList.AddRect(Rect{ glm::vec2(48.0f, 220.0f), glm::vec2(220.0f, 120.0f) }, Raised, CornerRadii{ 40.0f, 4.0f, 40.0f, 4.0f });
		drawList.AddRectBorder(Rect{ glm::vec2(48.0f, 220.0f), glm::vec2(220.0f, 120.0f) }, Outline, 1.0f, CornerRadii{ 40.0f, 4.0f, 40.0f, 4.0f });

		// Borders at whole device pixels, from a hairline up, to check the snapping holds
		for (int index = 0; index < 5; ++index)
		{
			drawList.AddRectBorder(Rect{ glm::vec2(300.0f + static_cast<float>(index) * 96.0f, 220.0f), glm::vec2(84.0f, 120.0f) }, Ink, 0.5f + static_cast<float>(index), CornerRadii::Uniform(8.0f));
		}

		// The bindless path, through slot 0's white. The editor's viewport image is the same call with a real slot
		drawList.AddImage(Rect{ glm::vec2(48.0f, 370.0f), glm::vec2(220.0f, 110.0f) }, Ignition::UI::WhiteTextureSlot, glm::vec4(0.85f, 0.35f, 0.15f, 1.0f));
	}

	void ClippingPage::OnPaint(Ignition::UI::DrawList& drawList)
	{
		const glm::vec2 surface = GetContext() ? GetContext()->GetSurfaceSize() : glm::vec2(0.0f);

		drawList.AddRect(Rect{ glm::vec2(0.0f), surface }, Surface);

		const Rect outer{ glm::vec2(60.0f, 90.0f), glm::vec2(420.0f, 300.0f) };

		drawList.AddRect(outer, Raised, CornerRadii::Uniform(10.0f));
		drawList.PushClipRect(outer);

		// A band that runs well past the clip on both sides, so a scissor off by a pixel is obvious
		for (int index = 0; index < 14; ++index)
		{
			drawList.AddRect(Rect{ glm::vec2(-40.0f + static_cast<float>(index) * 42.0f, 60.0f + Pulse(m_Phase + static_cast<float>(index) * 0.3f, -30.0f, 30.0f)), glm::vec2(28.0f, 420.0f) }, Accent, CornerRadii::Uniform(6.0f));
		}

		// Nested: the inner clip must end up as the intersection, never as a replacement
		const Rect inner{ glm::vec2(160.0f, 160.0f), glm::vec2(420.0f, 120.0f) };

		drawList.PushClipRect(inner);
		drawList.AddRect(Rect{ glm::vec2(0.0f), surface }, glm::vec4(0.95f, 0.75f, 0.15f, 0.85f));
		drawList.PopClipRect();

		drawList.PopClipRect();

		// Emitted before the content below it but at a higher layer, so Finish's sort is what puts it on top
		drawList.PushLayer(DrawLayer::Popup);
		drawList.AddShadow(Rect{ glm::vec2(360.0f, 300.0f), glm::vec2(240.0f, 140.0f) }, glm::vec4(0.0f, 0.0f, 0.0f, 0.6f), 24.0f, CornerRadii::Uniform(12.0f), glm::vec2(0.0f, 8.0f));
		drawList.AddRect(Rect{ glm::vec2(360.0f, 300.0f), glm::vec2(240.0f, 140.0f) }, glm::vec4(0.16f, 0.17f, 0.20f, 1.0f), CornerRadii::Uniform(12.0f));
		drawList.PopLayer();

		drawList.AddRect(Rect{ glm::vec2(300.0f, 340.0f), glm::vec2(240.0f, 140.0f) }, glm::vec4(0.15f, 0.55f, 0.35f, 1.0f), CornerRadii::Uniform(12.0f));
	}

	void TessellatorPage::OnPaint(Ignition::UI::DrawList& drawList)
	{
		const glm::vec2 surface = GetContext() ? GetContext()->GetSurfaceSize() : glm::vec2(0.0f);

		drawList.AddRect(Rect{ glm::vec2(0.0f), surface }, Surface);

		Ignition::UI::Tessellator tessellator;
		tessellator.SetFlatteningTolerance(0.2f);

		// A curve that changes shape every frame, so the flattening is re-run rather than cached into looking correct
		tessellator.MoveTo(glm::vec2(60.0f, 300.0f));
		tessellator.CubicTo(glm::vec2(160.0f, 100.0f + Pulse(m_Phase, -60.0f, 60.0f)), glm::vec2(300.0f, 460.0f), glm::vec2(420.0f, 240.0f));
		tessellator.Stroke(drawList, Ink, 6.0f, Ignition::UI::Tessellator::LineJoin::Miter, Ignition::UI::Tessellator::LineCap::Round);

		// A ring by two sub-paths of opposite winding: the hole is the winding rule's job, not a second draw
		tessellator.Reset();
		tessellator.Circle(glm::vec2(560.0f, 220.0f), 90.0f);
		tessellator.MoveTo(glm::vec2(560.0f + 46.0f, 220.0f));

		for (int index = 24; index >= 0; --index)
		{
			const float angle = 6.28318530718f * static_cast<float>(index) / 24.0f;

			tessellator.LineTo(glm::vec2(560.0f, 220.0f) + glm::vec2(std::cos(angle), std::sin(angle)) * 46.0f);
		}

		tessellator.Close();
		tessellator.Fill(drawList, Accent, Ignition::UI::Tessellator::FillRule::NonZero);

		// A rotated star: the rotation happens at point emission, on the CPU, and costs the GPU nothing
		tessellator.Reset();
		tessellator.SetTransform(glm::vec2(760.0f, 220.0f), m_Phase * 0.6f);

		for (int index = 0; index < 10; ++index)
		{
			const float angle = 6.28318530718f * static_cast<float>(index) / 10.0f;
			const float radius = index % 2 == 0 ? 80.0f : 34.0f;
			const glm::vec2 point = glm::vec2(std::cos(angle), std::sin(angle)) * radius;

			index == 0 ? tessellator.MoveTo(point) : tessellator.LineTo(point);
		}

		tessellator.Close();
		tessellator.Fill(drawList, glm::vec4(0.95f, 0.75f, 0.15f, 1.0f));
		tessellator.SetTransform(glm::vec2(0.0f));

		// Hairlines down to a third of a pixel: below one they hold their width and lose alpha instead of dropping out
		for (int index = 0; index < 6; ++index)
		{
			const float thickness = 0.33f + static_cast<float>(index) * 0.4f;

			drawList.AddLine(glm::vec2(60.0f, 380.0f + static_cast<float>(index) * 18.0f), glm::vec2(420.0f, 380.0f + static_cast<float>(index) * 18.0f), Ink, thickness);
		}

		// The two shapes the phase is judged on, at the size it is judged at
		const glm::vec2 chevron = glm::vec2(500.0f, 400.0f);
		const glm::vec2 chevronPoints[3] = { chevron + glm::vec2(0.0f, 0.0f), chevron + glm::vec2(8.0f, 8.0f), chevron + glm::vec2(0.0f, 16.0f) };

		drawList.AddPolyline(chevronPoints, 3, Ink, 2.0f, false);
		drawList.AddCircle(glm::vec2(560.0f, 408.0f), 8.0f, Ink, 2.0f);

		const float scales[3] = { 1.0f, 1.5f, 2.0f };
		float offset = 0.0f;

		for (const float scale : scales)
		{
			drawList.PushTransform(glm::vec2(640.0f + offset, 390.0f), glm::vec2(scale));

			const glm::vec2 marks[3] = { { 0.0f, 0.0f }, { 8.0f, 8.0f }, { 0.0f, 16.0f } };

			drawList.AddPolyline(marks, 3, Ink, 2.0f, false);
			drawList.AddCircle(glm::vec2(32.0f, 8.0f), 8.0f, Ink, 2.0f);
			drawList.PopTransform();

			offset += 60.0f * scale;
		}
	}

	SceneTargetPage::SceneTargetPage(Ignition::Renderer* renderer) : m_Renderer(renderer)
	{
		SetName("Scene Target");
	}

	SceneTargetPage::~SceneTargetPage()
	{
		// Zero releases the target and its slot through the frame gate, and puts the scene back on the swapchain
		if (m_Renderer)
		{
			m_Renderer->SetSceneRenderTargetSize(0, 0);
		}
	}

	void SceneTargetPage::OnTick(float deltaTime)
	{
		if (!m_Renderer)
		{
			return;
		}

		m_Phase += deltaTime;

		const int step = static_cast<int>(m_Phase / 0.75f) % 2;

		if (step != m_Step)
		{
			m_Step = step;

			m_Renderer->SetSceneRenderTargetSize(step == 0 ? 960u : 640u, step == 0 ? 540u : 360u);
		}

		const uint64_t slot = m_Renderer->GetSceneRenderTargetTextureID();

		if (slot != m_Slot)
		{
			m_Slot = slot;

			IG_APP_INFO("Scene target slot: {} ({}x{})", m_Slot, m_Renderer->GetSceneRenderTargetWidth(), m_Renderer->GetSceneRenderTargetHeight());
		}
	}

	void SceneTargetPage::OnPaint(Ignition::UI::DrawList& drawList)
	{
		const glm::vec2 surface = GetContext() ? GetContext()->GetSurfaceSize() : glm::vec2(0.0f);

		drawList.AddRect(Rect{ glm::vec2(0.0f), surface }, Surface);

		const Rect frame{ glm::vec2(60.0f, 90.0f), glm::vec2(960.0f, 540.0f) };

		// A checkerboard behind it, so the image's own extent and its alpha are both visible
		for (int row = 0; row < 18; ++row)
		{
			for (int column = 0; column < 32; ++column)
			{
				if ((row + column) % 2 == 0)
				{
					continue;
				}

				drawList.AddRect(Rect{ frame.Position + glm::vec2(static_cast<float>(column) * 30.0f, static_cast<float>(row) * 30.0f), glm::vec2(30.0f) }, Raised);
			}
		}

		drawList.AddImage(frame, static_cast<uint32_t>(m_Slot));
		drawList.AddRectBorder(frame, m_Slot == 0 ? glm::vec4(0.85f, 0.20f, 0.15f, 1.0f) : Accent, 2.0f, {});
		drawList.AddRect(Rect{ glm::vec2(60.0f, 650.0f), glm::vec2(static_cast<float>(m_Slot == 0 ? 0 : 240), 12.0f) }, Accent, CornerRadii::Uniform(6.0f));
	}

	void StressPage::OnPaint(Ignition::UI::DrawList& drawList)
	{
		const glm::vec2 surface = GetContext() ? GetContext()->GetSurfaceSize() : glm::vec2(0.0f);

		drawList.AddRect(Rect{ glm::vec2(0.0f), surface }, Surface);

		const int count = 1000 + static_cast<int>(Pulse(m_Phase * 0.5f, 0.0f, 11000.0f));
		const int columns = 160;
		const float cell = std::max(surface.x - 40.0f, 1.0f) / static_cast<float>(columns);

		// Radius animates so nothing can be cached, and every rect is its own primitive in the std430 buffer
		for (int index = 0; index < count; ++index)
		{
			const float x = 20.0f + static_cast<float>(index % columns) * cell;
			const float y = 60.0f + static_cast<float>(index / columns) * cell;

			if (y > surface.y)
			{
				break;
			}

			drawList.AddRect(Rect{ glm::vec2(x, y), glm::vec2(cell - 2.0f) }, index % 3 == 0 ? Accent : Ink, CornerRadii::Uniform(Pulse(m_Phase + static_cast<float>(index) * 0.01f, 0.0f, cell * 0.5f)));
		}
	}
}