#pragma once

#include <glm/vec2.hpp>

#include <cstdint>
#include <limits>

namespace Ignition
{
	namespace UI
	{
		// Axis-aligned rectangle in surface pixels, positioned by its top-left corner
		struct Rect
		{
			glm::vec2 Position{ 0.0f };
			glm::vec2 Size{ 0.0f };

			float GetLeft() const { return Position.x; }
			float GetTop() const { return Position.y; }
			float GetRight() const { return Position.x + Size.x; }
			float GetBottom() const { return Position.y + Size.y; }

			bool IsEmpty() const { return Size.x <= 0.0f || Size.y <= 0.0f; }

			// Half-open on the right and bottom edges so adjacent rects never both claim a pixel
			bool Contains(const glm::vec2& point) const
			{
				return point.x >= GetLeft() && point.x < GetRight() && point.y >= GetTop() && point.y < GetBottom();
			}
		};

		// What a parent offers a child during measure. Maximum is Unbounded on an axis the parent sizes to content
		struct Constraints
		{
			static constexpr float Unbounded = std::numeric_limits<float>::infinity();

			glm::vec2 Minimum{ 0.0f };
			glm::vec2 Maximum{ Unbounded, Unbounded };
		};

		// Measure implies arrange implies paint, never the reverse. Phase 3 gives each flag its propagation rule
		enum class DirtyFlags : uint32_t
		{
			None = 0,
			Paint = 1u << 0,
			Arrange = 1u << 1,
			Measure = 1u << 2,

			All = Paint | Arrange | Measure
		};

		constexpr DirtyFlags operator|(DirtyFlags left, DirtyFlags right)
		{
			return static_cast<DirtyFlags>(static_cast<uint32_t>(left) | static_cast<uint32_t>(right));
		}

		constexpr DirtyFlags operator&(DirtyFlags left, DirtyFlags right)
		{
			return static_cast<DirtyFlags>(static_cast<uint32_t>(left) & static_cast<uint32_t>(right));
		}

		constexpr DirtyFlags& operator|=(DirtyFlags& left, DirtyFlags right)
		{
			left = left | right;

			return left;
		}

		constexpr bool HasFlag(DirtyFlags value, DirtyFlags flag)
		{
			return (value & flag) != DirtyFlags::None;
		}
	}
}