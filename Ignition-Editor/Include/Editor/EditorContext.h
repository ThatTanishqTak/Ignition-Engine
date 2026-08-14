#pragma once

#include <Ignition/Scene/Entity.h>

#include <glm/vec2.hpp>

#include <string>

namespace Ignition
{
	class Scene;
}

namespace Editor
{
	struct EditorContext
	{
		Ignition::Scene* Scene = nullptr;
		Ignition::Entity Selection;
		std::string ScenePath;

		bool ViewportHovered = false;
		bool ViewportFocused = false;
		bool GizmoUsing = false;
		glm::vec2 ViewportPosition{ 0.0f };
		glm::vec2 ViewportSize{ 0.0f };
	};
}