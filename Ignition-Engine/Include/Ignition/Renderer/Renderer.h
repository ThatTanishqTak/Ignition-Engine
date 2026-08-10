#pragma once

#include "Ignition/Core/Export.h"
#include "Ignition/Renderer/Vertex.h"

#include <glm/mat4x4.hpp>

#include <memory>
#include <vector>

struct SDL_Window;

namespace Ignition
{
	class VulkanRenderer;
	class Camera;
	class Mesh;

	class Renderer
	{
	public:
		Renderer();
		~Renderer();

		void Initialize(SDL_Window* window);
		void Shutdown();

		IGNITION_API bool IsValid() const;

		IGNITION_API void SetClearColor(float r, float g, float b, float a = 1.0f);

		void BeginFrame();
		void EndFrame();

		void WaitIdle();

		IGNITION_API std::shared_ptr<Mesh> CreateMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

		IGNITION_API void BeginScene(const Camera& camera);
		IGNITION_API void Submit(const std::shared_ptr<Mesh>& mesh, const glm::mat4& transform);
		IGNITION_API void EndScene();

		void OnResize();

	private:
		std::unique_ptr<VulkanRenderer> m_VulkanRenderer;
	};
}