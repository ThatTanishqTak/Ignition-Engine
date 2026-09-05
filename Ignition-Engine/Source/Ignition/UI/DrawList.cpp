#include "Ignition/UI/DrawList.h"

#include "Ignition/Core/ProfilerInternal.h"
#include "Ignition/UI/Tessellator.h"

#include <glm/common.hpp>

#include <algorithm>
#include <cmath>

namespace Ignition
{
	namespace UI
	{
		namespace
		{
			constexpr float EdgeSoftness = 0.5f; // half the smoothstep band: one device pixel of AA

			bool SameClip(const Rect& left, const Rect& right)
			{
				return left.Position == right.Position && left.Size == right.Size;
			}

			Rect Intersect(const Rect& left, const Rect& right)
			{
				const float x = std::max(left.GetLeft(), right.GetLeft());
				const float y = std::max(left.GetTop(), right.GetTop());
				const float r = std::min(left.GetRight(), right.GetRight());
				const float b = std::min(left.GetBottom(), right.GetBottom());

				return Rect{ glm::vec2(x, y), glm::vec2(std::max(r - x, 0.0f), std::max(b - y, 0.0f)) };
			}

			glm::vec4 ClampRadii(const CornerRadii& radii, const glm::vec2& halfExtent)
			{
				const float limit = std::max(std::min(halfExtent.x, halfExtent.y), 0.0f);

				return glm::vec4(std::clamp(radii.TopLeft, 0.0f, limit), std::clamp(radii.TopRight, 0.0f, limit), std::clamp(radii.BottomRight, 0.0f, limit), std::clamp(radii.BottomLeft, 0.0f, limit));
			}

			CornerRadii ScaleRadii(const CornerRadii& radii, float scale)
			{
				return CornerRadii{ radii.TopLeft * scale, radii.TopRight * scale, radii.BottomRight * scale, radii.BottomLeft * scale };
			}

			CornerRadii InsetRadii(const CornerRadii& radii, float inset)
			{
				return CornerRadii{ std::max(radii.TopLeft - inset, 0.0f), std::max(radii.TopRight - inset, 0.0f), std::max(radii.BottomRight - inset, 0.0f), std::max(radii.BottomLeft - inset, 0.0f) };
			}

			Rect Expand(const Rect& rect, float padding)
			{
				return Rect{ rect.Position - glm::vec2(padding), rect.Size + glm::vec2(padding * 2.0f) };
			}
		}

		void DrawList::Clear(const glm::vec2& surfaceSize)
		{
			m_Vertices.clear();
			m_Indices.clear();
			m_Primitives.clear();
			m_Commands.clear();

			m_ClipStack.assign(1, Rect{ glm::vec2(0.0f), glm::max(surfaceSize, glm::vec2(0.0f)) });
			m_TransformStack.assign(1, DrawTransform{});
			m_LayerStack.assign(1, static_cast<uint32_t>(DrawLayer::Content));

			m_Current = DrawCommand{};
			m_CommandOpen = false;
			m_SolidPrimitiveValid = false;
		}

		void DrawList::OpenCommand()
		{
			const Rect& clip = m_ClipStack.back();
			const uint32_t layer = m_LayerStack.back();

			if (m_CommandOpen && m_Current.Layer == layer && SameClip(m_Current.ClipRect, clip))
			{
				return;
			}

			CloseCommand();

			m_Current = DrawCommand{ clip, layer, static_cast<uint32_t>(m_Indices.size()), 0 };
			m_CommandOpen = true;
		}

		void DrawList::CloseCommand()
		{
			if (!m_CommandOpen)
			{
				return;
			}

			m_Current.IndexCount = static_cast<uint32_t>(m_Indices.size()) - m_Current.IndexOffset;
			m_CommandOpen = false;

			// A clip change that emitted nothing leaves no command behind
			if (m_Current.IndexCount > 0)
			{
				m_Commands.push_back(m_Current);
			}
		}

		void DrawList::Finish()
		{
			IG_PROFILE_ZONE_NAMED("UI DrawList Finish");

			CloseCommand();

			std::stable_sort(m_Commands.begin(), m_Commands.end(), [](const DrawCommand& left, const DrawCommand& right)
			{
				return left.Layer < right.Layer;
			});

			std::vector<DrawCommand> merged;
			merged.reserve(m_Commands.size());

			for (const DrawCommand& command : m_Commands)
			{
				if (!merged.empty())
				{
					DrawCommand& previous = merged.back();

					if (previous.Layer == command.Layer && SameClip(previous.ClipRect, command.ClipRect) && previous.IndexOffset + previous.IndexCount == command.IndexOffset)
					{
						previous.IndexCount += command.IndexCount;

						continue;
					}
				}

				merged.push_back(command);
			}

			m_Commands.swap(merged);
		}

		void DrawList::PushClipRect(const Rect& rect, bool intersectWithCurrent)
		{
			const Rect transformed = TransformRect(rect);

			m_ClipStack.push_back(intersectWithCurrent ? Intersect(m_ClipStack.back(), transformed) : transformed);
		}

		void DrawList::PopClipRect()
		{
			if (m_ClipStack.size() > 1)
			{
				m_ClipStack.pop_back();
			}
		}

		void DrawList::PushTransform(const glm::vec2& translation, const glm::vec2& scale)
		{
			const DrawTransform& parent = m_TransformStack.back();

			// Composed, so the stack top is always the full surface transform and TransformPoint stays one multiply-add
			m_TransformStack.push_back(DrawTransform{ parent.Translation + parent.Scale * translation, parent.Scale * scale });
		}

		void DrawList::PopTransform()
		{
			if (m_TransformStack.size() > 1)
			{
				m_TransformStack.pop_back();
			}
		}

		void DrawList::PushLayer(uint32_t layer)
		{
			m_LayerStack.push_back(layer);
		}

		void DrawList::PopLayer()
		{
			if (m_LayerStack.size() > 1)
			{
				m_LayerStack.pop_back();
			}
		}

		DrawList::GeometryRange DrawList::AllocateGeometry(uint32_t vertexCount, uint32_t indexCount)
		{
			if (vertexCount == 0 || indexCount == 0 || m_ClipStack.back().IsEmpty())
			{
				return GeometryRange{};
			}

			OpenCommand();

			const uint32_t baseVertex = static_cast<uint32_t>(m_Vertices.size());
			const size_t baseIndex = m_Indices.size();

			m_Vertices.resize(baseVertex + vertexCount);
			m_Indices.resize(baseIndex + indexCount);

			return GeometryRange{ m_Vertices.data() + baseVertex, m_Indices.data() + baseIndex, baseVertex };
		}

		uint32_t DrawList::AddPrimitive(const DrawPrimitive& primitive)
		{
			m_Primitives.push_back(primitive);

			return static_cast<uint32_t>(m_Primitives.size() - 1);
		}

		uint32_t DrawList::GetSolidPrimitive()
		{
			if (!m_SolidPrimitiveValid)
			{
				m_SolidPrimitive = AddPrimitive(DrawPrimitive{});
				m_SolidPrimitiveValid = true;
			}

			return m_SolidPrimitive;
		}

		void DrawList::AddQuad(const Rect& rect, const Rect& uv, const glm::vec4& color, uint32_t primitive)
		{
			const GeometryRange range = AllocateGeometry(4, 6);

			if (!range.Vertices)
			{
				return;
			}

			const glm::vec2 corners[4] = { { rect.GetLeft(), rect.GetTop() }, { rect.GetRight(), rect.GetTop() }, { rect.GetRight(), rect.GetBottom() }, { rect.GetLeft(), rect.GetBottom() } };
			const glm::vec2 texture[4] = { { uv.GetLeft(), uv.GetTop() }, { uv.GetRight(), uv.GetTop() }, { uv.GetRight(), uv.GetBottom() }, { uv.GetLeft(), uv.GetBottom() } };

			for (int corner = 0; corner < 4; ++corner)
			{
				range.Vertices[corner] = DrawVertex{ corners[corner], texture[corner], color, primitive };
			}

			const uint32_t order[6] = { 0, 1, 2, 0, 2, 3 };

			for (int index = 0; index < 6; ++index)
			{
				range.Indices[index] = range.BaseVertex + order[index];
			}
		}

		void DrawList::EmitRounded(const Rect& rect, const glm::vec4& color, const CornerRadii& radii, float ringHalfWidth, float softness, float padding)
		{
			if (rect.Size.x <= 0.0f || rect.Size.y <= 0.0f || color.a <= 0.0f)
			{
				return;
			}

			const glm::vec2 halfExtent = rect.Size * 0.5f;

			DrawPrimitive primitive;
			primitive.Bounds = glm::vec4(rect.Position + halfExtent, halfExtent);
			primitive.Radius = ClampRadii(radii, halfExtent);
			primitive.Edge = glm::vec2(ringHalfWidth, softness);
			primitive.Mode = static_cast<uint32_t>(DrawMode::RoundedRect);

			// The quad has to hold the whole smoothstep tail, or the AA band gets cut off at the geometric edge
			AddQuad(Expand(rect, padding), Rect{ glm::vec2(0.0f), glm::vec2(1.0f) }, color, AddPrimitive(primitive));
		}

		void DrawList::AddRect(const Rect& rect, const glm::vec4& color, const CornerRadii& radii)
		{
			const DrawTransform& transform = m_TransformStack.back();
			const float scale = std::min(transform.Scale.x, transform.Scale.y);

			EmitRounded(TransformRect(rect), color, ScaleRadii(radii, scale), 0.0f, EdgeSoftness, 1.0f);
		}

		void DrawList::AddRectBorder(const Rect& rect, const glm::vec4& color, float thickness, const CornerRadii& radii)
		{
			const DrawTransform& transform = m_TransformStack.back();
			const float scale = std::min(transform.Scale.x, transform.Scale.y);
			const float width = Tessellator::SnapStrokeWidth(TransformScalar(thickness), m_DeviceScale);
			const float half = width * 0.5f;

			const Rect bounds = TransformRect(rect);

			if (bounds.Size.x <= width || bounds.Size.y <= width)
			{
				// Degenerate: the border swallows the box, so draw it as a fill rather than an inverted ring
				EmitRounded(bounds, color, ScaleRadii(radii, scale), 0.0f, EdgeSoftness, 1.0f);

				return;
			}

			const glm::vec2 topLeft(Tessellator::SnapCentreline(bounds.GetLeft() + half, width), Tessellator::SnapCentreline(bounds.GetTop() + half, width));
			const glm::vec2 bottomRight(Tessellator::SnapCentreline(bounds.GetRight() - half, width), Tessellator::SnapCentreline(bounds.GetBottom() - half, width));

			EmitRounded(Rect{ topLeft, bottomRight - topLeft }, color, InsetRadii(ScaleRadii(radii, scale), half), half, EdgeSoftness, half + 1.0f);
		}

		void DrawList::AddShadow(const Rect& rect, const glm::vec4& color, float blur, const CornerRadii& radii, const glm::vec2& offset)
		{
			const DrawTransform& transform = m_TransformStack.back();
			const float scale = std::min(transform.Scale.x, transform.Scale.y);
			const float softness = std::max(TransformScalar(blur), 0.0f) * 0.5f + EdgeSoftness;

			Rect shifted = rect;
			shifted.Position += offset;

			// Same quad, same SDF, same primitive struct as the fill - only Edge.y grew
			EmitRounded(TransformRect(shifted), color, ScaleRadii(radii, scale), 0.0f, softness, softness * 2.0f + 1.0f);
		}

		void DrawList::AddImage(const Rect& rect, uint32_t textureSlot, const glm::vec4& tint, const Rect& uv)
		{
			const Rect bounds = TransformRect(rect);

			if (bounds.IsEmpty() || tint.a <= 0.0f)
			{
				return;
			}

			const glm::vec2 halfExtent = bounds.Size * 0.5f;

			DrawPrimitive primitive;
			primitive.Bounds = glm::vec4(bounds.Position + halfExtent, halfExtent);
			primitive.Mode = static_cast<uint32_t>(DrawMode::Texture);
			primitive.Texture = textureSlot;

			AddQuad(bounds, uv, tint, AddPrimitive(primitive));
		}

		void DrawList::AddLine(const glm::vec2& from, const glm::vec2& to, const glm::vec4& color, float thickness)
		{
			const glm::vec2 points[2] = { TransformPoint(from), TransformPoint(to) };

			Tessellator::StrokePolyline(*this, points, 2, false, color, Tessellator::SnapStrokeWidth(TransformScalar(thickness), m_DeviceScale));
		}

		void DrawList::AddPolyline(const glm::vec2* points, size_t count, const glm::vec4& color, float thickness, bool closed)
		{
			if (!points || count < 2)
			{
				return;
			}

			m_Scratch.clear();
			m_Scratch.reserve(count);

			for (size_t point = 0; point < count; ++point)
			{
				m_Scratch.push_back(TransformPoint(points[point]));
			}

			Tessellator::StrokePolyline(*this, m_Scratch.data(), m_Scratch.size(), closed, color, Tessellator::SnapStrokeWidth(TransformScalar(thickness), m_DeviceScale));
		}

		void DrawList::AddConvexPolyFilled(const glm::vec2* points, size_t count, const glm::vec4& color)
		{
			if (!points || count < 3)
			{
				return;
			}

			m_Scratch.clear();
			m_Scratch.reserve(count);

			for (size_t point = 0; point < count; ++point)
			{
				m_Scratch.push_back(TransformPoint(points[point]));
			}

			Tessellator::FillConvex(*this, m_Scratch.data(), m_Scratch.size(), color);
		}

		void DrawList::AddCircle(const glm::vec2& centre, float radius, const glm::vec4& color, float thickness)
		{
			// A ring is the SDF path, not the tessellator's: exact at any radius and one quad either way
			const Rect bounds{ centre - glm::vec2(radius), glm::vec2(radius * 2.0f) };

			AddRectBorder(bounds, color, thickness, CornerRadii::Uniform(radius));
		}

		void DrawList::AddCircleFilled(const glm::vec2& centre, float radius, const glm::vec4& color)
		{
			const Rect bounds{ centre - glm::vec2(radius), glm::vec2(radius * 2.0f) };

			AddRect(bounds, color, CornerRadii::Uniform(radius));
		}
	}
}