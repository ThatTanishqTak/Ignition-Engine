#pragma once

#include <Ignition/UI/DrawList.h>
#include <Ignition/UI/Element.h>

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Ignition
{
	class Renderer;
}

namespace Sandbox
{
	class NavStripElement final : public Ignition::UI::Element
	{
	public:
		static constexpr float Height = 34.0f;
		static constexpr float TabWidth = 168.0f;

		NavStripElement(std::vector<std::string> names, std::function<void(size_t)> onSelect);

		void SetActive(size_t index) { m_Active = index; }
		void SetPointer(const glm::vec2& position) { m_Pointer = position; }

		// True when the press landed on a tab, so the caller knows whether to keep looking
		bool OnPointerDown(const glm::vec2& position);

		Ignition::UI::Rect GetTabRect(size_t index) const;

		const char* GetTypeName() const override { return "NavStrip"; }

	protected:
		void OnPaint(Ignition::UI::DrawList& drawList) override;

	private:
		std::vector<std::string> m_Names;
		std::function<void(size_t)> m_Select;
		glm::vec2 m_Pointer{ -1.0f };
		size_t m_Active = 0;
	};

	class PrimitivesPage final : public Ignition::UI::Element
	{
	public:
		const char* GetTypeName() const override { return "PrimitivesPage"; }

	protected:
		void OnTick(float deltaTime) override { m_Phase += deltaTime; }
		void OnPaint(Ignition::UI::DrawList& drawList) override;

	private:
		float m_Phase = 0.0f;
	};

	// Nested clips and the layer table, in one frame, so a wrong scissor or a wrong sort is visible rather than subtle
	class ClippingPage final : public Ignition::UI::Element
	{
	public:
		const char* GetTypeName() const override { return "ClippingPage"; }

	protected:
		void OnTick(float deltaTime) override { m_Phase += deltaTime; }
		void OnPaint(Ignition::UI::DrawList& drawList) override;

	private:
		float m_Phase = 0.0f;
	};

	// The tessellator under load: curves, a hole, thin strokes, and the two 16 px shapes the phase is judged on
	class TessellatorPage final : public Ignition::UI::Element
	{
	public:
		const char* GetTypeName() const override { return "TessellatorPage"; }

	protected:
		void OnTick(float deltaTime) override { m_Phase += deltaTime; }
		void OnPaint(Ignition::UI::DrawList& drawList) override;

	private:
		float m_Phase = 0.0f;
	};

	class SceneTargetPage final : public Ignition::UI::Element
	{
	public:
		explicit SceneTargetPage(Ignition::Renderer* renderer);
		~SceneTargetPage() override;

		const char* GetTypeName() const override { return "SceneTargetPage"; }

	protected:
		void OnTick(float deltaTime) override;
		void OnPaint(Ignition::UI::DrawList& drawList) override;

	private:
		Ignition::Renderer* m_Renderer = nullptr;
		float m_Phase = 0.0f;
		int m_Step = -1;
		uint64_t m_Slot = 0;
	};

	class StressPage final : public Ignition::UI::Element
	{
	public:
		const char* GetTypeName() const override { return "StressPage"; }

	protected:
		void OnTick(float deltaTime) override { m_Phase += deltaTime; }
		void OnPaint(Ignition::UI::DrawList& drawList) override;

	private:
		float m_Phase = 0.0f;
	};
}