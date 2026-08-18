#pragma once

#include "Ignition/Fluid/FluidSolver3D.h"
#include "Ignition/Renderer/Vulkan/VulkanBuffer.h"
#include "Ignition/Renderer/Vulkan/VulkanComputePass.h"
#include "Ignition/Renderer/Vulkan/VulkanComputePipeline.h"
#include "Ignition/Renderer/Vulkan/VulkanFrameContext.h"

#include <vulkan/vulkan.h>

#include <glm/vec3.hpp>

#include <array>
#include <cstdint>
#include <string>

namespace Ignition
{
	class VulkanDescriptorAllocator;
	class VulkanGPUTimer;
	struct FluidPushConstants3D;

	class VulkanFluidSolver3D final : public VulkanComputePass
	{
	public:
		static constexpr uint32_t ReductionGroups = 64;

		VulkanFluidSolver3D();
		~VulkanFluidSolver3D() override;

		VulkanFluidSolver3D(const VulkanFluidSolver3D&) = delete;
		VulkanFluidSolver3D& operator=(const VulkanFluidSolver3D&) = delete;

		void Initialize(VkDevice device, VmaAllocator allocator, VulkanDescriptorAllocator& descriptorAllocator, const std::string& spirvPath, const FluidSolver3DSettings& settings);
		void Shutdown();

		bool IsValid() const;

		// Live parameters only - the resolution is fixed at Initialize
		void Configure(const FluidSolver3DSettings& settings);

		void RequestReset() { m_ResetRequested = true; }
		void RequestSteps(uint32_t steps) { m_PendingSteps += steps; }

		void RecordCompute(VkCommandBuffer commandBuffer, uint32_t frameIndex, VulkanGPUTimer* timer) override;

		uint64_t GetStepCount() const { return m_StepCount; }
		glm::vec3 GetLatticeForce() const { return m_LatticeForce; }
		glm::vec3 GetLatticeTorque() const { return m_LatticeTorque; }
		uint32_t GetProjectedCellCount() const { return m_ProjectedCells; }

	private:
		bool CreateBuffers();
		bool CreateDescriptors();
		bool CreatePipelines(const std::string& spirvPath);

		FluidPushConstants3D BuildParameters() const;

		void Dispatch(VkCommandBuffer commandBuffer, const VulkanComputePipeline& pipeline, VkDescriptorSet descriptorSet, uint32_t groupsX, uint32_t groupsY = 1, uint32_t groupsZ = 1) const;
		void CopyResults(VkCommandBuffer commandBuffer, uint32_t frameIndex);
		void ReadResults(uint32_t frameIndex);

	private:
		VkDevice m_Device = VK_NULL_HANDLE;
		VmaAllocator m_Allocator = VK_NULL_HANDLE;
		VulkanDescriptorAllocator* m_DescriptorAllocator = nullptr;

		FluidSolver3DSettings m_Settings;
		FluidLatticeScaling m_Scaling;

		std::array<VulkanBuffer, 2> m_Distributions;
		VulkanBuffer m_Flags;
		VulkanBuffer m_Fields;
		VulkanBuffer m_CellForces;    // structure-of-arrays, three components; the moment arm is the cell's own index, so torque needs no per-cell storage
		VulkanBuffer m_ForcePartials; // six components per reduction group
		VulkanBuffer m_ForceResult;
		VulkanBuffer m_Area;
		std::array<VulkanBuffer, VulkanFrameContext::MaximumFramesInFlight> m_Readback;
		std::array<bool, VulkanFrameContext::MaximumFramesInFlight> m_ReadbackPending{};

		VkDescriptorSetLayout m_SetLayout = VK_NULL_HANDLE;
		std::array<VkDescriptorSet, 2> m_DescriptorSets{};

		VulkanComputePipeline m_InitializePipeline;
		VulkanComputePipeline m_ProjectedAreaPipeline;
		VulkanComputePipeline m_StreamCollidePipeline;
		VulkanComputePipeline m_ReducePartialPipeline;
		VulkanComputePipeline m_ReduceFinalPipeline;

		uint32_t m_CurrentSet = 0;
		uint32_t m_PendingSteps = 0;
		bool m_ResetRequested = true;
		uint64_t m_StepCount = 0;

		glm::vec3 m_LatticeForce{ 0.0f };
		glm::vec3 m_LatticeTorque{ 0.0f };
		uint32_t m_ProjectedCells = 0;
	};
}