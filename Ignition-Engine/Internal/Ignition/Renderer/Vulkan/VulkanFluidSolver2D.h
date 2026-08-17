#pragma once

#include "Ignition/Fluid/FluidSolver2D.h"
#include "Ignition/Renderer/Vulkan/VulkanBuffer.h"
#include "Ignition/Renderer/Vulkan/VulkanComputePipeline.h"
#include "Ignition/Renderer/Vulkan/VulkanFrameContext.h"

#include <vulkan/vulkan.h>

#include <glm/vec2.hpp>

#include <array>
#include <cstdint>
#include <memory>
#include <string>

namespace Ignition
{
	class VulkanDescriptorAllocator;
	class VulkanGPUTimer;
	class VulkanImage;
	class VulkanRenderer;

	class VulkanFluidSolver2D
	{
	public:
		static constexpr uint32_t ReductionGroups = 64;

		VulkanFluidSolver2D();
		~VulkanFluidSolver2D();

		VulkanFluidSolver2D(const VulkanFluidSolver2D&) = delete;
		VulkanFluidSolver2D& operator=(const VulkanFluidSolver2D&) = delete;

		void Initialize(VulkanRenderer& renderer, VkDevice device, VmaAllocator allocator, VulkanDescriptorAllocator& descriptorAllocator, const std::string& spirvPath, const FluidSolver2DSettings& settings);
		void Shutdown();

		bool IsValid() const;

		// Live parameters only - the resolution is fixed at Initialize
		void Configure(const FluidSolver2DSettings& settings);

		void RequestReset() { m_ResetRequested = true; }
		void RequestSteps(uint32_t steps) { m_PendingSteps += steps; }

		void SetVisualizationField(FluidField field) { m_Field = field; }
		void SetColorScale(float scale) { m_ColorScale = scale; }

		// Recorded by the renderer before the scene pass opens - compute cannot run inside dynamic rendering
		void RecordCompute(VkCommandBuffer commandBuffer, uint32_t frameIndex, VulkanGPUTimer* timer);

		uint64_t GetVisualizationTextureID() const { return reinterpret_cast<uint64_t>(m_ImGuiTexture); }
		uint64_t GetStepCount() const { return m_StepCount; }
		glm::vec2 GetLatticeForce() const { return m_LatticeForce; }

	private:
		bool CreateBuffers();
		bool CreateDescriptors();
		bool CreatePipelines(const std::string& spirvPath);

		void DispatchGrid(VkCommandBuffer commandBuffer, const VulkanComputePipeline& pipeline, VkDescriptorSet descriptorSet) const;
		void DispatchLinear(VkCommandBuffer commandBuffer, const VulkanComputePipeline& pipeline, VkDescriptorSet descriptorSet, uint32_t groups) const;

		void TransitionVisualization(VkCommandBuffer commandBuffer, bool toStorage);
		void ReadForce(uint32_t frameIndex);

	private:
		VkDevice m_Device = VK_NULL_HANDLE;
		VmaAllocator m_Allocator = VK_NULL_HANDLE;
		VulkanDescriptorAllocator* m_DescriptorAllocator = nullptr;
		std::weak_ptr<VulkanRenderer*> m_Renderer;

		FluidSolver2DSettings m_Settings;
		FluidLatticeScaling m_Scaling;

		std::array<VulkanBuffer, 2> m_Distributions;
		VulkanBuffer m_Flags;
		VulkanBuffer m_Fields;
		VulkanBuffer m_CellForces;
		VulkanBuffer m_ForcePartials;
		VulkanBuffer m_ForceResult;
		std::array<VulkanBuffer, VulkanFrameContext::MaximumFramesInFlight> m_ForceReadback;
		std::array<bool, VulkanFrameContext::MaximumFramesInFlight> m_ForceReadbackPending{};

		std::unique_ptr<VulkanImage> m_Visualization;
		VkDescriptorSet m_ImGuiTexture = VK_NULL_HANDLE;
		bool m_VisualizationInitialized = false;

		VkDescriptorSetLayout m_SetLayout = VK_NULL_HANDLE;
		std::array<VkDescriptorSet, 2> m_DescriptorSets{};

		VulkanComputePipeline m_InitializePipeline;
		VulkanComputePipeline m_StreamCollidePipeline;
		VulkanComputePipeline m_VisualizePipeline;
		VulkanComputePipeline m_ReducePartialPipeline;
		VulkanComputePipeline m_ReduceFinalPipeline;

		FluidField m_Field = FluidField::Vorticity;
		float m_ColorScale = 1.0f;

		uint32_t m_CurrentSet = 0;
		uint32_t m_PendingSteps = 0;
		bool m_ResetRequested = true;
		uint64_t m_StepCount = 0;
		glm::vec2 m_LatticeForce{ 0.0f };
	};
}