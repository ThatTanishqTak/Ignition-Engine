#pragma once

#include "Ignition/Core/Export.h"
#include "Ignition/UI/UITypes.h"

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include <cstdint>
#include <vector>

namespace Ignition
{
	namespace UI
	{
		// Slot 0 of the texture table is a permanent 1x1 white, so an untextured primitive still has something to name
		inline constexpr uint32_t WhiteTextureSlot = 0;

		// Mirrors UI.slang's Mode switch
		enum class DrawMode : uint32_t
		{
			Solid = 0,
			RoundedRect = 1,
			Texture = 2,
			GlyphCoverage = 3, // Phase 2
			GlyphSDF = 4       // Phase 10, and only if D9's condition fires
		};

		// §7's layer table. Inside a layer emission order decides; across layers the layer decides
		enum class DrawLayer : uint32_t
		{
			Content = 0,
			Chrome = 100,
			Popup = 200,
			Tooltip = 300,
			DragPreview = 400,
			Debug = 500
		};

		struct CornerRadii
		{
			float TopLeft = 0.0f;
			float TopRight = 0.0f;
			float BottomRight = 0.0f;
			float BottomLeft = 0.0f;

			static constexpr CornerRadii Uniform(float radius) { return { radius, radius, radius, radius }; }

			bool IsZero() const { return TopLeft <= 0.0f && TopRight <= 0.0f && BottomRight <= 0.0f && BottomLeft <= 0.0f; }
		};

		// 36 bytes. Positions are final surface pixels: the transform stack is applied at emission, on the CPU
		struct DrawVertex
		{
			glm::vec2 Position{ 0.0f };
			glm::vec2 UV{ 0.0f };
			glm::vec4 Color{ 1.0f }; // linear, straight alpha - the fragment premultiplies
			uint32_t Primitive = 0;
		};

		// std430, 48 bytes, laid out to match UI.slang's UIPrimitive with no implicit padding
		struct DrawPrimitive
		{
			glm::vec4 Bounds{ 0.0f };
			glm::vec4 Radius{ 0.0f };
			glm::vec2 Edge{ 0.0f };
			uint32_t Mode = static_cast<uint32_t>(DrawMode::Solid);
			uint32_t Texture = WhiteTextureSlot;
		};

		struct DrawCommand
		{
			Rect ClipRect;
			uint32_t Layer = 0;
			uint32_t IndexOffset = 0;
			uint32_t IndexCount = 0;
		};

		// Translate and scale only. Rotation is the tessellator's job, applied at point emission, so hit testing can
		// reuse this arithmetic verbatim in Phase 4
		struct DrawTransform
		{
			glm::vec2 Translation{ 0.0f };
			glm::vec2 Scale{ 1.0f };
		};

		class DrawList
		{
		public:
			// Vertices and Indices are null when the current clip rect is empty - always check before writing
			struct GeometryRange
			{
				DrawVertex* Vertices = nullptr;
				uint32_t* Indices = nullptr;
				uint32_t BaseVertex = 0;
			};

			IGNITION_API void Clear(const glm::vec2& surfaceSize);
			IGNITION_API void Finish();

			bool IsEmpty() const { return m_Commands.empty(); }

			const std::vector<DrawVertex>& GetVertices() const { return m_Vertices; }
			const std::vector<uint32_t>& GetIndices() const { return m_Indices; }
			const std::vector<DrawPrimitive>& GetPrimitives() const { return m_Primitives; }
			const std::vector<DrawCommand>& GetCommands() const { return m_Commands; }

			// Whole device pixels after DPI scaling, with a 1.0 px floor, is what keeps a 1 px border 1 px
			void SetDeviceScale(float scale) { m_DeviceScale = scale > 0.0f ? scale : 1.0f; }
			float GetDeviceScale() const { return m_DeviceScale; }

			IGNITION_API void PushClipRect(const Rect& rect, bool intersectWithCurrent = true);
			IGNITION_API void PopClipRect();
			const Rect& GetClipRect() const { return m_ClipStack.back(); }

			IGNITION_API void PushTransform(const glm::vec2& translation, const glm::vec2& scale = glm::vec2(1.0f));
			IGNITION_API void PopTransform();
			const DrawTransform& GetTransform() const { return m_TransformStack.back(); }

			glm::vec2 TransformPoint(const glm::vec2& point) const
			{
				const DrawTransform& transform = m_TransformStack.back();

				return transform.Translation + transform.Scale * point;
			}

			Rect TransformRect(const Rect& rect) const
			{
				const DrawTransform& transform = m_TransformStack.back();

				return Rect{ transform.Translation + transform.Scale * rect.Position, transform.Scale * rect.Size };
			}

			// The smaller axis, so a stroke under a non-uniform scale never fattens on one side only
			float TransformScalar(float value) const
			{
				const DrawTransform& transform = m_TransformStack.back();

				return value * (transform.Scale.x < transform.Scale.y ? transform.Scale.x : transform.Scale.y);
			}

			IGNITION_API void PushLayer(uint32_t layer);
			void PushLayer(DrawLayer layer) { PushLayer(static_cast<uint32_t>(layer)); }
			IGNITION_API void PopLayer();
			uint32_t GetLayer() const { return m_LayerStack.back(); }

			IGNITION_API void AddRect(const Rect& rect, const glm::vec4& color, const CornerRadii& radii = {});
			IGNITION_API void AddRectBorder(const Rect& rect, const glm::vec4& color, float thickness, const CornerRadii& radii = {});
			IGNITION_API void AddShadow(const Rect& rect, const glm::vec4& color, float blur, const CornerRadii& radii = {}, const glm::vec2& offset = glm::vec2(0.0f));
			IGNITION_API void AddImage(const Rect& rect, uint32_t textureSlot, const glm::vec4& tint = glm::vec4(1.0f), const Rect& uv = Rect{ glm::vec2(0.0f), glm::vec2(1.0f) });

			IGNITION_API void AddLine(const glm::vec2& from, const glm::vec2& to, const glm::vec4& color, float thickness = 1.0f);
			IGNITION_API void AddPolyline(const glm::vec2* points, size_t count, const glm::vec4& color, float thickness, bool closed);
			IGNITION_API void AddConvexPolyFilled(const glm::vec2* points, size_t count, const glm::vec4& color);
			IGNITION_API void AddCircle(const glm::vec2& centre, float radius, const glm::vec4& color, float thickness = 1.0f);
			IGNITION_API void AddCircleFilled(const glm::vec2& centre, float radius, const glm::vec4& color);

			IGNITION_API GeometryRange AllocateGeometry(uint32_t vertexCount, uint32_t indexCount);
			IGNITION_API uint32_t AddPrimitive(const DrawPrimitive& primitive);
			IGNITION_API uint32_t GetSolidPrimitive();
			IGNITION_API void AddQuad(const Rect& rect, const Rect& uv, const glm::vec4& color, uint32_t primitive);

		private:
			void OpenCommand();
			void CloseCommand();
			void EmitRounded(const Rect& rect, const glm::vec4& color, const CornerRadii& radii, float ringHalfWidth, float softness, float padding);

		private:
			std::vector<DrawVertex> m_Vertices;
			std::vector<uint32_t> m_Indices;
			std::vector<DrawPrimitive> m_Primitives;
			std::vector<DrawCommand> m_Commands;

			std::vector<Rect> m_ClipStack;
			std::vector<DrawTransform> m_TransformStack;
			std::vector<uint32_t> m_LayerStack;
			std::vector<glm::vec2> m_Scratch;

			DrawCommand m_Current;
			bool m_CommandOpen = false;

			uint32_t m_SolidPrimitive = 0;
			bool m_SolidPrimitiveValid = false;

			float m_DeviceScale = 1.0f;
		};
	}
}