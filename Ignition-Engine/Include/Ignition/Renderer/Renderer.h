#pragma once

#include "Ignition/Core/Export.h"
#include "Ignition/Renderer/Vertex.h"

#include <glm/mat4x4.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Ignition
{
	class Camera;
	class Mesh;
	class Texture;
	struct Material;
	struct RendererImplementation;

	// One GPU pass, timed by timestamp queries, resolved a frame late
	struct PassTiming
	{
		std::string Name;
		float Milliseconds = 0.0f;
	};

	class Renderer
	{
	public:
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&) = delete;

		IGNITION_API bool IsValid() const;

		IGNITION_API void SetClearColor(float r, float g, float b, float a = 1.0f);

		IGNITION_API bool IsImGuiFrameActive() const;

		IGNITION_API bool WantCaptureMouse() const;
		IGNITION_API bool WantCaptureKeyboard() const;

		IGNITION_API std::shared_ptr<Mesh> CreateMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
		IGNITION_API std::shared_ptr<Texture> CreateTexture(const std::string& filepath);
		IGNITION_API std::shared_ptr<Texture> CreateTextureFromMemory(const void* data, size_t size);

		IGNITION_API void BeginScene(const Camera& camera);
		IGNITION_API void Submit(const std::shared_ptr<Mesh>& mesh, const glm::mat4& transform);
		IGNITION_API void Submit(const std::shared_ptr<Mesh>& mesh, const Material& material, const glm::mat4& transform);
		IGNITION_API void EndScene();
		
		IGNITION_API void SetSceneRenderTargetSize(uint32_t width, uint32_t height);
		IGNITION_API uint64_t GetSceneRenderTargetTextureID() const;
		IGNITION_API uint32_t GetSceneRenderTargetWidth() const;
		IGNITION_API uint32_t GetSceneRenderTargetHeight() const;

		IGNITION_API const std::vector<PassTiming>& GetPassTimings() const;

	private:
		friend class Engine;
		friend class FluidSolver3D;

		Renderer();

		template <typename TResource, typename TBackendHandle>
		std::shared_ptr<TResource> WrapBackendResource(std::unique_ptr<TBackendHandle> handle);

		std::unique_ptr<RendererImplementation> m_Implementation;
	};
}