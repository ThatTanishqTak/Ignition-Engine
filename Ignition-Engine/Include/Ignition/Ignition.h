#pragma once

// Core
#include "Ignition/Core/Application.h"
#include "Ignition/Core/Layer.h"
#include "Ignition/Core/LayerStack.h"
#include "Ignition/Core/Log.h"
#include "Ignition/Core/Time.h"

// Events
#include "Ignition/Events/Event.h"
#include "Ignition/Events/EventQueue.h"
#include "Ignition/Events/KeyEvent.h"
#include "Ignition/Events/MouseEvent.h"
#include "Ignition/Events/WindowEvent.h"
#include "Ignition/Events/GamepadEvent.h"

// Event codes
#include "Ignition/Events/KeyCodes.h"
#include "Ignition/Events/ScanCodes.h"
#include "Ignition/Events/MouseCodes.h"
#include "Ignition/Events/GamepadCodes.h"

// Input
#include "Ignition/Input/Input.h"
#include "Ignition/Input/ActionMap.h"
#include "Ignition/Input/CursorMode.h"

// Renderer
#include "Ignition/Renderer/Renderer.h"
#include "Ignition/Renderer/Camera.h"
#include "Ignition/Renderer/Mesh.h"
#include "Ignition/Renderer/Texture.h"
#include "Ignition/Renderer/Material.h"
#include "Ignition/Renderer/Vertex.h"
#include "Ignition/Renderer/DebugDraw.h"

// Assets
#include "Ignition/Assets/AssetRegistry.h"

// Physics
#include "Ignition/Physics/PhysicsComponents.h"
#include "Ignition/Physics/PhysicsWorld.h"

// Fluid
#include "Ignition/Fluid/FluidSolver2D.h"

// Scene
#include "Ignition/Scene/Scene.h"
#include "Ignition/Scene/Entity.h"
#include "Ignition/Scene/Components.h"
#include "Ignition/Scene/ModelImporter.h"
#include "Ignition/Scene/SceneSerializer.h"

// UI
#include "Ignition/UI/UI.h"

// Window
#include "Ignition/Window/Window.h"