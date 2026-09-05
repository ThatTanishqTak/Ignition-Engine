#include "Ignition/UI/Tessellator.h"

#include "Ignition/Core/ProfilerInternal.h"
#include "Ignition/UI/DrawList.h"

#include <glm/geometric.hpp>

#include <algorithm>
#include <cmath>

namespace Ignition
{
	namespace UI
	{
		namespace
		{
			constexpr float Fringe = 0.5f; // half the AA band, in device pixels
			constexpr int MaximumSubdivisionDepth = 16;

			float SignedArea(const glm::vec2* points, size_t count)
			{
				float area = 0.0f;

				for (size_t index = 0; index < count; ++index)
				{
					const glm::vec2& current = points[index];
					const glm::vec2& next = points[(index + 1) % count];

					area += current.x * next.y - next.x * current.y;
				}

				return area * 0.5f;
			}

			// Left of the edge for a positive-area contour is the interior in y-down surface space, so this is outward
			glm::vec2 EdgeNormal(const glm::vec2& from, const glm::vec2& to)
			{
				const glm::vec2 direction = to - from;
				const float length = glm::length(direction);

				return length > 1e-6f ? glm::vec2(direction.y, -direction.x) / length : glm::vec2(0.0f);
			}

			// |miter| is 1 / cos(theta / 2), which is exactly the offset a mitred join needs
			glm::vec2 MiterNormal(const glm::vec2& previous, const glm::vec2& current, const glm::vec2& next, float limit, bool& outBeveled)
			{
				const glm::vec2 incoming = EdgeNormal(previous, current);
				const glm::vec2 outgoing = EdgeNormal(current, next);
				const glm::vec2 sum = incoming + outgoing;
				const float lengthSquared = glm::dot(sum, sum);

				if (lengthSquared < 1e-12f)
				{
					// A 180 degree reversal has no miter at all
					outBeveled = true;

					return incoming;
				}

				const glm::vec2 miter = sum * (2.0f / lengthSquared);

				outBeveled = glm::length(miter) > limit;

				return miter;
			}

			bool PointInTriangle(const glm::vec2& point, const glm::vec2& a, const glm::vec2& b, const glm::vec2& c)
			{
				const auto side = [](const glm::vec2& p, const glm::vec2& from, const glm::vec2& to)
				{
					return (to.x - from.x) * (p.y - from.y) - (to.y - from.y) * (p.x - from.x);
				};

				const float first = side(point, a, b);
				const float second = side(point, b, c);
				const float third = side(point, c, a);

				return (first >= 0.0f && second >= 0.0f && third >= 0.0f) || (first <= 0.0f && second <= 0.0f && third <= 0.0f);
			}

			bool PointInContour(const glm::vec2& point, const std::vector<glm::vec2>& contour)
			{
				bool inside = false;

				for (size_t index = 0, previous = contour.size() - 1; index < contour.size(); previous = index++)
				{
					const glm::vec2& current = contour[index];
					const glm::vec2& last = contour[previous];

					if ((current.y > point.y) != (last.y > point.y) && point.x < (last.x - current.x) * (point.y - current.y) / (last.y - current.y) + current.x)
					{
						inside = !inside;
					}
				}

				return inside;
			}

			bool SegmentsCross(const glm::vec2& a0, const glm::vec2& a1, const glm::vec2& b0, const glm::vec2& b1)
			{
				const auto orientation = [](const glm::vec2& p, const glm::vec2& q, const glm::vec2& r)
				{
					const float value = (q.x - p.x) * (r.y - p.y) - (q.y - p.y) * (r.x - p.x);

					return value > 1e-6f ? 1 : (value < -1e-6f ? -1 : 0);
				};

				return orientation(a0, a1, b0) * orientation(a0, a1, b1) < 0 && orientation(b0, b1, a0) * orientation(b0, b1, a1) < 0;
			}

			// Ear clipping over a polygon already normalised to positive signed area
			void EarClip(const std::vector<glm::vec2>& polygon, std::vector<uint32_t>& outIndices)
			{
				const size_t count = polygon.size();

				if (count < 3)
				{
					return;
				}

				std::vector<uint32_t> remaining(count);

				for (size_t index = 0; index < count; ++index)
				{
					remaining[index] = static_cast<uint32_t>(index);
				}

				size_t guard = count * count;

				while (remaining.size() > 2 && guard-- > 0)
				{
					bool clipped = false;

					for (size_t index = 0; index < remaining.size(); ++index)
					{
						const uint32_t previous = remaining[(index + remaining.size() - 1) % remaining.size()];
						const uint32_t current = remaining[index];
						const uint32_t next = remaining[(index + 1) % remaining.size()];

						const glm::vec2& a = polygon[previous];
						const glm::vec2& b = polygon[current];
						const glm::vec2& c = polygon[next];

						if ((b.x - a.x) * (c.y - b.y) - (b.y - a.y) * (c.x - b.x) <= 0.0f)
						{
							continue;
						}

						bool contains = false;

						for (const uint32_t candidate : remaining)
						{
							if (candidate == previous || candidate == current || candidate == next)
							{
								continue;
							}

							if (PointInTriangle(polygon[candidate], a, b, c))
							{
								contains = true;

								break;
							}
						}

						if (contains)
						{
							continue;
						}

						outIndices.push_back(previous);
						outIndices.push_back(current);
						outIndices.push_back(next);

						remaining.erase(remaining.begin() + static_cast<std::ptrdiff_t>(index));
						clipped = true;

						break;
					}

					if (!clipped)
					{
						// Self-intersecting or degenerate input: emit what is left as a fan rather than spinning
						for (size_t index = 1; index + 1 < remaining.size(); ++index)
						{
							outIndices.push_back(remaining[0]);
							outIndices.push_back(remaining[index]);
							outIndices.push_back(remaining[index + 1]);
						}

						return;
					}
				}
			}

			void BridgeHole(std::vector<glm::vec2>& outer, const std::vector<glm::vec2>& hole, const std::vector<std::vector<glm::vec2>>& obstacles)
			{
				size_t holeStart = 0;

				for (size_t index = 1; index < hole.size(); ++index)
				{
					if (hole[index].x < hole[holeStart].x)
					{
						holeStart = index;
					}
				}

				size_t bridge = outer.size();
				float best = std::numeric_limits<float>::max();

				for (size_t index = 0; index < outer.size(); ++index)
				{
					const float distance = glm::distance(outer[index], hole[holeStart]);

					if (distance >= best)
					{
						continue;
					}

					bool blocked = false;

					for (const std::vector<glm::vec2>& obstacle : obstacles)
					{
						for (size_t edge = 0; edge < obstacle.size() && !blocked; ++edge)
						{
							blocked = SegmentsCross(outer[index], hole[holeStart], obstacle[edge], obstacle[(edge + 1) % obstacle.size()]);
						}

						if (blocked)
						{
							break;
						}
					}

					if (!blocked)
					{
						best = distance;
						bridge = index;
					}
				}

				if (bridge >= outer.size())
				{
					return;
				}

				std::vector<glm::vec2> merged;
				merged.reserve(outer.size() + hole.size() + 2);

				merged.insert(merged.end(), outer.begin(), outer.begin() + static_cast<std::ptrdiff_t>(bridge) + 1);

				for (size_t step = 0; step <= hole.size(); ++step)
				{
					merged.push_back(hole[(holeStart + step) % hole.size()]);
				}

				merged.insert(merged.end(), outer.begin() + static_cast<std::ptrdiff_t>(bridge), outer.end());

				outer.swap(merged);
			}

			// One AA ring around a contour: solid on the boundary, transparent one fringe outward
			void EmitFringe(DrawList& drawList, const std::vector<glm::vec2>& contour, const glm::vec4& color, float orientation, float miterLimit)
			{
				const size_t count = contour.size();

				if (count < 3)
				{
					return;
				}

				const DrawList::GeometryRange range = drawList.AllocateGeometry(static_cast<uint32_t>(count * 2), static_cast<uint32_t>(count * 6));

				if (!range.Vertices)
				{
					return;
				}

				const uint32_t primitive = drawList.GetSolidPrimitive();
				glm::vec4 transparent = color;
				transparent.a = 0.0f;

				for (size_t index = 0; index < count; ++index)
				{
					bool beveled = false;
					const glm::vec2 normal = MiterNormal(contour[(index + count - 1) % count], contour[index], contour[(index + 1) % count], miterLimit, beveled) * orientation;

					range.Vertices[index * 2 + 0] = DrawVertex{ contour[index], glm::vec2(0.0f), color, primitive };
					range.Vertices[index * 2 + 1] = DrawVertex{ contour[index] + normal * (Fringe * 2.0f), glm::vec2(0.0f), transparent, primitive };
				}

				for (size_t index = 0; index < count; ++index)
				{
					const uint32_t inner = range.BaseVertex + static_cast<uint32_t>(index * 2);
					const uint32_t outer = inner + 1;
					const uint32_t nextInner = range.BaseVertex + static_cast<uint32_t>(((index + 1) % count) * 2);
					const uint32_t nextOuter = nextInner + 1;

					uint32_t* target = range.Indices + index * 6;

					target[0] = inner;
					target[1] = outer;
					target[2] = nextOuter;
					target[3] = inner;
					target[4] = nextOuter;
					target[5] = nextInner;
				}
			}
		}

		void Tessellator::Reset()
		{
			m_Points.clear();
			m_SubPaths.clear();
		}

		void Tessellator::SetTransform(const glm::vec2& translation, float rotationRadians, const glm::vec2& scale)
		{
			m_Translation = translation;
			m_Scale = scale;
			m_Sine = std::sin(rotationRadians);
			m_Cosine = std::cos(rotationRadians);
		}

		glm::vec2 Tessellator::Apply(const glm::vec2& point) const
		{
			const glm::vec2 scaled = point * m_Scale;

			return m_Translation + glm::vec2(scaled.x * m_Cosine - scaled.y * m_Sine, scaled.x * m_Sine + scaled.y * m_Cosine);
		}

		Tessellator::SubPath& Tessellator::Current()
		{
			if (m_SubPaths.empty())
			{
				m_SubPaths.push_back(SubPath{ m_Points.size(), 0, false });
			}

			return m_SubPaths.back();
		}

		void Tessellator::Emit(const glm::vec2& point)
		{
			SubPath& subPath = Current();

			// Collapse a repeat: a zero-length edge has no normal and would poison every join that touches it
			if (subPath.Count > 0 && glm::distance(m_Points.back(), point) < 1e-4f)
			{
				return;
			}

			m_Points.push_back(point);
			++subPath.Count;
		}

		void Tessellator::MoveTo(const glm::vec2& point)
		{
			m_SubPaths.push_back(SubPath{ m_Points.size(), 0, false });

			Emit(Apply(point));
		}

		void Tessellator::LineTo(const glm::vec2& point)
		{
			Emit(Apply(point));
		}

		void Tessellator::QuadraticTo(const glm::vec2& control, const glm::vec2& end)
		{
			if (m_Points.empty())
			{
				MoveTo(control);
			}

			std::vector<glm::vec2> flattened;
			FlattenQuadratic(flattened, m_Points.back(), Apply(control), Apply(end), m_Tolerance);

			for (const glm::vec2& point : flattened)
			{
				Emit(point);
			}
		}

		void Tessellator::CubicTo(const glm::vec2& first, const glm::vec2& second, const glm::vec2& end)
		{
			if (m_Points.empty())
			{
				MoveTo(first);
			}

			std::vector<glm::vec2> flattened;
			FlattenCubic(flattened, m_Points.back(), Apply(first), Apply(second), Apply(end), m_Tolerance);

			for (const glm::vec2& point : flattened)
			{
				Emit(point);
			}
		}

		void Tessellator::ArcTo(const glm::vec2& centre, float radius, float startRadians, float endRadians)
		{
			std::vector<glm::vec2> flattened;
			FlattenArc(flattened, centre, radius, startRadians, endRadians, m_Tolerance);

			for (const glm::vec2& point : flattened)
			{
				if (m_Points.empty() || Current().Count == 0)
				{
					MoveTo(point);
				}
				else
				{
					Emit(Apply(point));
				}
			}
		}

		void Tessellator::Rectangle(const Rect& rect)
		{
			MoveTo(glm::vec2(rect.GetLeft(), rect.GetTop()));
			LineTo(glm::vec2(rect.GetRight(), rect.GetTop()));
			LineTo(glm::vec2(rect.GetRight(), rect.GetBottom()));
			LineTo(glm::vec2(rect.GetLeft(), rect.GetBottom()));
			Close();
		}

		void Tessellator::RoundedRectangle(const Rect& rect, const CornerRadii& radii)
		{
			const float limit = std::min(rect.Size.x, rect.Size.y) * 0.5f;
			const float topLeft = std::clamp(radii.TopLeft, 0.0f, limit);
			const float topRight = std::clamp(radii.TopRight, 0.0f, limit);
			const float bottomRight = std::clamp(radii.BottomRight, 0.0f, limit);
			const float bottomLeft = std::clamp(radii.BottomLeft, 0.0f, limit);

			constexpr float half = 1.57079632679f;

			MoveTo(glm::vec2(rect.GetLeft() + topLeft, rect.GetTop()));
			LineTo(glm::vec2(rect.GetRight() - topRight, rect.GetTop()));
			ArcTo(glm::vec2(rect.GetRight() - topRight, rect.GetTop() + topRight), topRight, -half, 0.0f);
			LineTo(glm::vec2(rect.GetRight(), rect.GetBottom() - bottomRight));
			ArcTo(glm::vec2(rect.GetRight() - bottomRight, rect.GetBottom() - bottomRight), bottomRight, 0.0f, half);
			LineTo(glm::vec2(rect.GetLeft() + bottomLeft, rect.GetBottom()));
			ArcTo(glm::vec2(rect.GetLeft() + bottomLeft, rect.GetBottom() - bottomLeft), bottomLeft, half, half * 2.0f);
			LineTo(glm::vec2(rect.GetLeft(), rect.GetTop() + topLeft));
			ArcTo(glm::vec2(rect.GetLeft() + topLeft, rect.GetTop() + topLeft), topLeft, half * 2.0f, half * 3.0f);
			Close();
		}

		void Tessellator::Circle(const glm::vec2& centre, float radius)
		{
			ArcTo(centre, radius, 0.0f, 6.28318530718f);
			Close();
		}

		void Tessellator::Close()
		{
			if (!m_SubPaths.empty())
			{
				m_SubPaths.back().Closed = true;
			}
		}

		void Tessellator::FlattenQuadratic(std::vector<glm::vec2>& out, const glm::vec2& start, const glm::vec2& control, const glm::vec2& end, float tolerance)
		{
			// Every quadratic is a cubic whose controls sit two thirds of the way along each leg
			FlattenCubic(out, start, start + (control - start) * (2.0f / 3.0f), end + (control - end) * (2.0f / 3.0f), end, tolerance);
		}

		void Tessellator::FlattenCubic(std::vector<glm::vec2>& out, const glm::vec2& start, const glm::vec2& first, const glm::vec2& second, const glm::vec2& end, float tolerance)
		{
			struct Segment
			{
				glm::vec2 Start;
				glm::vec2 First;
				glm::vec2 Second;
				glm::vec2 End;
				int Depth;
			};

			std::vector<Segment> stack;
			stack.push_back(Segment{ start, first, second, end, 0 });

			std::vector<glm::vec2> reversed;

			while (!stack.empty())
			{
				const Segment segment = stack.back();
				stack.pop_back();

				const glm::vec2 chord = segment.End - segment.Start;
				const float firstDistance = std::abs((segment.First.x - segment.End.x) * chord.y - (segment.First.y - segment.End.y) * chord.x);
				const float secondDistance = std::abs((segment.Second.x - segment.End.x) * chord.y - (segment.Second.y - segment.End.y) * chord.x);
				const float error = firstDistance + secondDistance;

				// Screen-space error, not parameter count: a flat curve gets two points however long it is
				if (segment.Depth >= MaximumSubdivisionDepth || error * error < tolerance * glm::dot(chord, chord))
				{
					reversed.push_back(segment.End);

					continue;
				}

				const glm::vec2 ab = (segment.Start + segment.First) * 0.5f;
				const glm::vec2 bc = (segment.First + segment.Second) * 0.5f;
				const glm::vec2 cd = (segment.Second + segment.End) * 0.5f;
				const glm::vec2 abc = (ab + bc) * 0.5f;
				const glm::vec2 bcd = (bc + cd) * 0.5f;
				const glm::vec2 middle = (abc + bcd) * 0.5f;

				stack.push_back(Segment{ middle, bcd, cd, segment.End, segment.Depth + 1 });
				stack.push_back(Segment{ segment.Start, ab, abc, middle, segment.Depth + 1 });
			}

			out.insert(out.end(), reversed.begin(), reversed.end());
		}

		void Tessellator::FlattenArc(std::vector<glm::vec2>& out, const glm::vec2& centre, float radius, float startRadians, float endRadians, float tolerance)
		{
			if (radius <= 0.0f)
			{
				out.push_back(centre);

				return;
			}

			const float sweep = endRadians - startRadians;

			// Segment count straight from the sagitta, so a big circle gets more segments and a 4 px corner gets three
			const float step = 2.0f * std::acos(std::clamp(1.0f - tolerance / radius, -1.0f, 1.0f));
			const int segments = std::max(2, static_cast<int>(std::ceil(std::abs(sweep) / std::max(step, 1e-3f))));

			for (int segment = 0; segment <= segments; ++segment)
			{
				const float angle = startRadians + sweep * (static_cast<float>(segment) / static_cast<float>(segments));

				out.push_back(centre + glm::vec2(std::cos(angle), std::sin(angle)) * radius);
			}
		}

		float Tessellator::SnapStrokeWidth(float width, float deviceScale)
		{
			const float device = std::round(width * (deviceScale > 0.0f ? deviceScale : 1.0f));

			return std::max(device, 1.0f);
		}

		float Tessellator::SnapCentreline(float coordinate, float width)
		{
			return static_cast<int>(std::round(width)) % 2 != 0 ? std::floor(coordinate) + 0.5f : std::round(coordinate);
		}

		void Tessellator::StrokePolyline(DrawList& drawList, const glm::vec2* points, size_t count, bool closed, const glm::vec4& color, float thickness, LineJoin join, LineCap cap, float miterLimit)
		{
			IG_PROFILE_ZONE_NAMED("UI Stroke");

			if (!points || count < 2 || color.a <= 0.0f)
			{
				return;
			}

			glm::vec4 tinted = color;
			float half = thickness * 0.5f;

			// Sub-pixel strokes stay one pixel wide and lose alpha instead, which reads as a thin line rather than a gap
			if (thickness < 1.0f)
			{
				tinted.a *= std::max(thickness, 0.0f);
				half = 0.5f;
			}

			struct Rib
			{
				glm::vec2 Position;
				glm::vec2 Normal;
				float Alpha;
			};

			std::vector<Rib> ribs;
			ribs.reserve(count * 2 + 2);

			const size_t last = count - 1;

			for (size_t index = 0; index < count; ++index)
			{
				const bool interior = closed || (index > 0 && index < last);

				if (!interior)
				{
					const glm::vec2 normal = index == 0 ? EdgeNormal(points[0], points[1]) : EdgeNormal(points[last - 1], points[last]);
					glm::vec2 position = points[index];

					if (cap == LineCap::Square)
					{
						const glm::vec2 tangent = glm::vec2(-normal.y, normal.x);

						position += (index == 0 ? -tangent : tangent) * half;
					}

					const glm::vec2 tangent = glm::vec2(-normal.y, normal.x);
					const glm::vec2 outward = (index == 0 ? -tangent : tangent) * (Fringe * 2.0f);

					if (index == 0)
					{
						ribs.push_back(Rib{ position + outward, normal, 0.0f });
						ribs.push_back(Rib{ position, normal, tinted.a });
					}
					else
					{
						ribs.push_back(Rib{ position, normal, tinted.a });
						ribs.push_back(Rib{ position + outward, normal, 0.0f });
					}

					continue;
				}

				const glm::vec2& previous = points[(index + count - 1) % count];
				const glm::vec2& current = points[index];
				const glm::vec2& next = points[(index + 1) % count];

				bool beveled = false;
				const glm::vec2 normal = MiterNormal(previous, current, next, miterLimit, beveled);

				if (join == LineJoin::Bevel || beveled)
				{
					ribs.push_back(Rib{ current, EdgeNormal(previous, current), tinted.a });
					ribs.push_back(Rib{ current, EdgeNormal(current, next), tinted.a });
				}
				else
				{
					ribs.push_back(Rib{ current, normal, tinted.a });
				}
			}

			if (closed && !ribs.empty())
			{
				ribs.push_back(ribs.front());
			}

			if (ribs.size() < 2)
			{
				return;
			}

			const uint32_t vertexCount = static_cast<uint32_t>(ribs.size() * 4);
			const uint32_t indexCount = static_cast<uint32_t>((ribs.size() - 1) * 18);

			const DrawList::GeometryRange range = drawList.AllocateGeometry(vertexCount, indexCount);

			if (!range.Vertices)
			{
				return;
			}

			const uint32_t primitive = drawList.GetSolidPrimitive();

			for (size_t index = 0; index < ribs.size(); ++index)
			{
				const Rib& rib = ribs[index];

				glm::vec4 solid = tinted;
				solid.a = rib.Alpha;

				glm::vec4 transparent = tinted;
				transparent.a = 0.0f;

				DrawVertex* vertex = range.Vertices + index * 4;

				vertex[0] = DrawVertex{ rib.Position + rib.Normal * (half + Fringe), glm::vec2(0.0f), transparent, primitive };
				vertex[1] = DrawVertex{ rib.Position + rib.Normal * std::max(half - Fringe, 0.0f), glm::vec2(0.0f), solid, primitive };
				vertex[2] = DrawVertex{ rib.Position - rib.Normal * std::max(half - Fringe, 0.0f), glm::vec2(0.0f), solid, primitive };
				vertex[3] = DrawVertex{ rib.Position - rib.Normal * (half + Fringe), glm::vec2(0.0f), transparent, primitive };
			}

			for (size_t index = 0; index + 1 < ribs.size(); ++index)
			{
				const uint32_t current = range.BaseVertex + static_cast<uint32_t>(index * 4);
				const uint32_t next = current + 4;

				uint32_t* target = range.Indices + index * 18;

				for (uint32_t band = 0; band < 3; ++band)
				{
					target[band * 6 + 0] = current + band;
					target[band * 6 + 1] = current + band + 1;
					target[band * 6 + 2] = next + band + 1;
					target[band * 6 + 3] = current + band;
					target[band * 6 + 4] = next + band + 1;
					target[band * 6 + 5] = next + band;
				}
			}

			if (cap == LineCap::Round && !closed)
			{
				std::vector<glm::vec2> disc;
				FlattenArc(disc, points[0], half, 0.0f, 6.28318530718f, 0.25f);
				FillConvex(drawList, disc.data(), disc.size(), tinted);

				disc.clear();
				FlattenArc(disc, points[last], half, 0.0f, 6.28318530718f, 0.25f);
				FillConvex(drawList, disc.data(), disc.size(), tinted);
			}
		}

		void Tessellator::FillConvex(DrawList& drawList, const glm::vec2* points, size_t count, const glm::vec4& color)
		{
			IG_PROFILE_ZONE_NAMED("UI Fill Convex");

			if (!points || count < 3 || color.a <= 0.0f)
			{
				return;
			}

			const float orientation = SignedArea(points, count) >= 0.0f ? 1.0f : -1.0f;
			const uint32_t vertexCount = static_cast<uint32_t>(count * 2);
			const uint32_t indexCount = static_cast<uint32_t>((count - 2) * 3 + count * 6);

			const DrawList::GeometryRange range = drawList.AllocateGeometry(vertexCount, indexCount);

			if (!range.Vertices)
			{
				return;
			}

			const uint32_t primitive = drawList.GetSolidPrimitive();

			glm::vec4 transparent = color;
			transparent.a = 0.0f;

			for (size_t index = 0; index < count; ++index)
			{
				bool beveled = false;
				const glm::vec2 normal = MiterNormal(points[(index + count - 1) % count], points[index], points[(index + 1) % count], 4.0f, beveled) * orientation;

				range.Vertices[index * 2 + 0] = DrawVertex{ points[index] - normal * Fringe, glm::vec2(0.0f), color, primitive };
				range.Vertices[index * 2 + 1] = DrawVertex{ points[index] + normal * Fringe, glm::vec2(0.0f), transparent, primitive };
			}

			uint32_t* target = range.Indices;

			for (size_t index = 1; index + 1 < count; ++index)
			{
				*target++ = range.BaseVertex;
				*target++ = range.BaseVertex + static_cast<uint32_t>(index * 2);
				*target++ = range.BaseVertex + static_cast<uint32_t>((index + 1) * 2);
			}

			for (size_t index = 0; index < count; ++index)
			{
				const uint32_t inner = range.BaseVertex + static_cast<uint32_t>(index * 2);
				const uint32_t outer = inner + 1;
				const uint32_t nextInner = range.BaseVertex + static_cast<uint32_t>(((index + 1) % count) * 2);
				const uint32_t nextOuter = nextInner + 1;

				*target++ = inner;
				*target++ = outer;
				*target++ = nextOuter;
				*target++ = inner;
				*target++ = nextOuter;
				*target++ = nextInner;
			}
		}

		void Tessellator::Stroke(DrawList& drawList, const glm::vec4& color, float thickness, LineJoin join, LineCap cap) const
		{
			for (const SubPath& subPath : m_SubPaths)
			{
				if (subPath.Count < 2)
				{
					continue;
				}

				StrokePolyline(drawList, m_Points.data() + subPath.First, subPath.Count, subPath.Closed, color, thickness, join, cap, m_MiterLimit);
			}
		}

		void Tessellator::Fill(DrawList& drawList, const glm::vec4& color, FillRule rule) const
		{
			IG_PROFILE_ZONE_NAMED("UI Fill Path");

			if (color.a <= 0.0f)
			{
				return;
			}

			std::vector<std::vector<glm::vec2>> contours;
			std::vector<float> areas;

			for (const SubPath& subPath : m_SubPaths)
			{
				if (subPath.Count < 3)
				{
					continue;
				}

				contours.emplace_back(m_Points.begin() + static_cast<std::ptrdiff_t>(subPath.First), m_Points.begin() + static_cast<std::ptrdiff_t>(subPath.First + subPath.Count));
				areas.push_back(SignedArea(contours.back().data(), contours.back().size()));
			}

			if (contours.empty())
			{
				return;
			}

			std::vector<int> parents(contours.size(), -1);
			std::vector<int> depths(contours.size(), 0);

			for (size_t index = 0; index < contours.size(); ++index)
			{
				for (size_t other = 0; other < contours.size(); ++other)
				{
					if (index == other || !PointInContour(contours[index].front(), contours[other]))
					{
						continue;
					}

					++depths[index];

					if (parents[index] < 0 || std::abs(areas[other]) < std::abs(areas[static_cast<size_t>(parents[index])]))
					{
						parents[index] = static_cast<int>(other);
					}
				}
			}

			std::vector<bool> holes(contours.size(), false);

			for (size_t index = 0; index < contours.size(); ++index)
			{
				if (rule == FillRule::EvenOdd)
				{
					holes[index] = depths[index] % 2 != 0;
				}
				else
				{
					holes[index] = parents[index] >= 0 && (areas[index] >= 0.0f) != (areas[static_cast<size_t>(parents[index])] >= 0.0f);
				}
			}

			for (size_t index = 0; index < contours.size(); ++index)
			{
				if (holes[index])
				{
					continue;
				}

				std::vector<glm::vec2> merged = contours[index];
				std::vector<std::vector<glm::vec2>> obstacles = { contours[index] };

				for (size_t candidate = 0; candidate < contours.size(); ++candidate)
				{
					if (!holes[candidate] || parents[candidate] != static_cast<int>(index))
					{
						continue;
					}

					obstacles.push_back(contours[candidate]);
					BridgeHole(merged, contours[candidate], obstacles);
				}

				if (SignedArea(merged.data(), merged.size()) < 0.0f)
				{
					std::reverse(merged.begin(), merged.end());
				}

				std::vector<uint32_t> triangles;
				EarClip(merged, triangles);

				if (triangles.empty())
				{
					continue;
				}

				const DrawList::GeometryRange range = drawList.AllocateGeometry(static_cast<uint32_t>(merged.size()), static_cast<uint32_t>(triangles.size()));

				if (!range.Vertices)
				{
					continue;
				}

				const uint32_t primitive = drawList.GetSolidPrimitive();

				for (size_t vertex = 0; vertex < merged.size(); ++vertex)
				{
					range.Vertices[vertex] = DrawVertex{ merged[vertex], glm::vec2(0.0f), color, primitive };
				}

				for (size_t triangle = 0; triangle < triangles.size(); ++triangle)
				{
					range.Indices[triangle] = range.BaseVertex + triangles[triangle];
				}

				EmitFringe(drawList, contours[index], color, areas[index] >= 0.0f ? 1.0f : -1.0f, m_MiterLimit);

				for (size_t candidate = 0; candidate < contours.size(); ++candidate)
				{
					if (holes[candidate] && parents[candidate] == static_cast<int>(index))
					{
						EmitFringe(drawList, contours[candidate], color, areas[candidate] >= 0.0f ? 1.0f : -1.0f, m_MiterLimit);
					}
				}
			}
		}
	}
}