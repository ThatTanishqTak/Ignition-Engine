#pragma once

#include "Ignition/Core/Export.h"
#include "Ignition/UI/UITypes.h"

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include <cstddef>
#include <vector>

namespace Ignition
{
	namespace UI
	{
		class DrawList;
		struct CornerRadii;

		class Tessellator
		{
		public:
			enum class FillRule
			{
				NonZero = 0,
				EvenOdd
			};

			enum class LineJoin
			{
				Miter = 0, // falls back to a bevel past the miter limit
				Bevel
			};

			enum class LineCap
			{
				Butt = 0,
				Square,
				Round
			};

			IGNITION_API void Reset();

			// Screen-space error budget for curve flattening, in device pixels
			void SetFlatteningTolerance(float tolerance) { m_Tolerance = tolerance > 0.0f ? tolerance : 0.25f; }
			void SetMiterLimit(float limit) { m_MiterLimit = limit > 1.0f ? limit : 1.0f; }

			// Applied as points are emitted, on the CPU. A rotated path costs nothing at draw time and nothing on the GPU
			IGNITION_API void SetTransform(const glm::vec2& translation, float rotationRadians = 0.0f, const glm::vec2& scale = glm::vec2(1.0f));

			IGNITION_API void MoveTo(const glm::vec2& point);
			IGNITION_API void LineTo(const glm::vec2& point);
			IGNITION_API void QuadraticTo(const glm::vec2& control, const glm::vec2& end);
			IGNITION_API void CubicTo(const glm::vec2& first, const glm::vec2& second, const glm::vec2& end);
			IGNITION_API void ArcTo(const glm::vec2& centre, float radius, float startRadians, float endRadians);
			IGNITION_API void Rectangle(const Rect& rect);
			IGNITION_API void RoundedRectangle(const Rect& rect, const CornerRadii& radii);
			IGNITION_API void Circle(const glm::vec2& centre, float radius);
			IGNITION_API void Close();

			IGNITION_API void Fill(DrawList& drawList, const glm::vec4& color, FillRule rule = FillRule::NonZero) const;
			IGNITION_API void Stroke(DrawList& drawList, const glm::vec4& color, float thickness, LineJoin join = LineJoin::Miter, LineCap cap = LineCap::Butt) const;

			IGNITION_API static void StrokePolyline(DrawList& drawList, const glm::vec2* points, size_t count, bool closed, const glm::vec4& color, float thickness, LineJoin join = LineJoin::Miter, LineCap cap = LineCap::Butt, float miterLimit = 4.0f);
			IGNITION_API static void FillConvex(DrawList& drawList, const glm::vec2* points, size_t count, const glm::vec4& color);

			IGNITION_API static void FlattenCubic(std::vector<glm::vec2>& out, const glm::vec2& start, const glm::vec2& first, const glm::vec2& second, const glm::vec2& end, float tolerance);
			IGNITION_API static void FlattenQuadratic(std::vector<glm::vec2>& out, const glm::vec2& start, const glm::vec2& control, const glm::vec2& end, float tolerance);
			IGNITION_API static void FlattenArc(std::vector<glm::vec2>& out, const glm::vec2& centre, float radius, float startRadians, float endRadians, float tolerance);

			// Quality pass: whole device pixels with a 1 px floor, and a centreline snapped so an odd width lands on a
			// half pixel and an even one on a pixel boundary
			IGNITION_API static float SnapStrokeWidth(float width, float deviceScale);
			IGNITION_API static float SnapCentreline(float coordinate, float width);

		private:
			struct SubPath
			{
				size_t First = 0;
				size_t Count = 0;
				bool Closed = false;
			};

			glm::vec2 Apply(const glm::vec2& point) const;
			void Emit(const glm::vec2& point);
			SubPath& Current();

		private:
			std::vector<glm::vec2> m_Points;
			std::vector<SubPath> m_SubPaths;

			glm::vec2 m_Translation{ 0.0f };
			glm::vec2 m_Scale{ 1.0f };
			float m_Sine = 0.0f;
			float m_Cosine = 1.0f;

			float m_Tolerance = 0.25f;
			float m_MiterLimit = 4.0f;
		};
	}
}