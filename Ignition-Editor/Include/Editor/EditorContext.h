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
	enum class PlayState
	{
		Edit = 0,
		Playing,
		Paused
	};

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

		PlayState Play = PlayState::Edit;
		bool StepRequested = false;

		bool DrawColliders = true;
		bool DrawPhysXVisualization = false;

		// Set whenever colliders or transforms change so the edit-mode query scene can be rebuilt
		bool PhysicsSceneDirty = true;
	};
}