#pragma once

#include "Ignition/Fluid/FluidSolver3D.h"
#include "Ignition/Renderer/Vulkan/VulkanBuffer.h"
#include "Ignition/Renderer/Vulkan/VulkanComputePass.h"
#include "Ignition/Renderer/Vulkan/VulkanComputePipeline.h"
#include "Ignition/Renderer/Vulkan/VulkanFrameContext.h"

#include <vulkan/vulkan.h>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

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
	struct FluidPushConstants3D;

	class VulkanFluidSolver3D final : public VulkanComputePass
	{
	public:
		static constexpr uint32_t ReductionGroups = 64;

		VulkanFluidSolver3D();
		~VulkanFluidSolver3D() override;

		VulkanFluidSolver3D(const VulkanFluidSolver3D&) = delete;
		VulkanFluidSolver3D& operator=(const VulkanFluidSolver3D&) = delete;

		void Initialize(VulkanRenderer& renderer, VkDevice device, VkQueue graphicsQueue, uint32_t graphicsQueueFamily, VmaAllocator allocator, VulkanDescriptorAllocator& descriptorAllocator, const std::string& spirvPath, const FluidSolver3DSettings& settings);
		void Shutdown();

		bool IsValid() const;

		// Live parameters only - the resolution is fixed at Initialize
		void Configure(const FluidSolver3DSettings& settings);

		void RequestReset() { m_ResetRequested = true; }
		void RequestSteps(uint32_t steps) { m_PendingSteps += steps; }

		void SetBodies(const std::vector<FluidBody>& bodies);
		FluidVoxelStatus GetVoxelStatus() const;

		void RecordCompute(VkCommandBuffer commandBuffer, uint32_t frameIndex, VulkanGPUTimer* timer) override;

		// Scene-pass hooks: the slice plane as a translucent quad, tracers and the pressure shell as GPU-resident lines
		bool GetSceneQuad(VulkanSceneQuad& quad) override;
		void RecordSceneLines(VkCommandBuffer commandBuffer, uint32_t frameIndex, VulkanLineRenderer& lines, const glm::mat4& viewProjection) override;

		uint64_t GetSliceTextureID() const { return m_SliceInitialized ? reinterpret_cast<uint64_t>(m_SliceImGuiTexture) : 0; }

		uint64_t GetStepCount() const { return m_StepCount; }
		glm::vec3 GetLatticeForce() const { return m_LatticeForce; }
		glm::vec3 GetLatticeTorque() const { return m_LatticeTorque; }
		uint32_t GetProjectedCellCount() const { return m_ProjectedCells; }

	private:
		bool CreateBuffers();
		bool CreateDescriptors();
		bool CreatePipelines(const std::string& spirvPath);

		FluidPushConstants3D BuildParameters() const;

		void UploadBodies();
		void RecordMask(VkCommandBuffer commandBuffer, const FluidPushConstants3D& parameters);

		// The lattice, not the requested box, is the domain: cubic cells mean the covered volume is Resolution * dx, floor on the world origin
		glm::vec3 LatticeExtent() const;
		glm::vec3 LatticeMinimum() const;

		void Dispatch(VkCommandBuffer commandBuffer, const VulkanComputePipeline& pipeline, VkDescriptorSet descriptorSet, const FluidPushConstants3D& parameters, uint32_t groupsX, uint32_t groupsY = 1, uint32_t groupsZ = 1) const;
		void RecordVisualization(VkCommandBuffer commandBuffer, const FluidPushConstants3D& parameters, uint32_t steps, VulkanGPUTimer* timer);
		void TransitionSlice(VkCommandBuffer commandBuffer, bool toStorage);
		void CopyResults(VkCommandBuffer commandBuffer, uint32_t frameIndex);
		void ReadResults(uint32_t frameIndex);

		glm::mat4 LatticeToWorld() const;

	private:
		struct VoxelBatch
		{
			uint32_t ObjectID = 1;
			uint32_t FirstTriangle = 0;
			uint32_t TriangleCount = 0;
		};

		VkDevice m_Device = VK_NULL_HANDLE;
		VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
		uint32_t m_GraphicsQueueFamily = 0;
		VmaAllocator m_Allocator = VK_NULL_HANDLE;
		VulkanDescriptorAllocator* m_DescriptorAllocator = nullptr;
		std::weak_ptr<VulkanRenderer*> m_Renderer;

		FluidSolver3DSettings m_Settings;
		FluidLatticeScaling m_Scaling;

		std::array<VulkanBuffer, 2> m_Distributions;
		VulkanBuffer m_Flags;
		VulkanBuffer m_Fields;
		VulkanBuffer m_CellForces;    // structure-of-arrays, three components; the moment arm is the cell's own index, so torque needs no per-cell storage
		VulkanBuffer m_ForcePartials; // six components per reduction group
		VulkanBuffer m_ForceResult;
		VulkanBuffer m_Counters; // projected columns, flood changes, rejected triangles, solid cells
		std::array<VulkanBuffer, VulkanFrameContext::MaximumFramesInFlight> m_Readback;
		std::array<bool, VulkanFrameContext::MaximumFramesInFlight> m_ReadbackPending{};

		// Voxelizer (Step 3.2)
		VulkanBuffer m_VoxelPositions;
		VulkanBuffer m_VoxelIndices;
		std::vector<FluidBody> m_Bodies;
		std::vector<VoxelBatch> m_VoxelBatches;
		std::vector<float> m_VoxelPositionData;
		std::vector<uint32_t> m_VoxelIndexData;
		uint32_t m_TriangleCount = 0;
		bool m_BodiesDirty = false;

		// Visualization (Step 3.4)
		VulkanBuffer m_Particles;
		VulkanBuffer m_ParticleVertices; // compute-written, drawn by the line pipeline - nothing is read back
		VulkanBuffer m_ShellVertices;
		VulkanBuffer m_ShellIndirect;    // VkDrawIndirectCommand appended to by the shell kernel

		std::unique_ptr<VulkanImage> m_Slice;
		VkSampler m_SliceSampler = VK_NULL_HANDLE;
		VkDescriptorSet m_SliceSceneTexture = VK_NULL_HANDLE; // texture-layout set for the in-scene quad
		VkDescriptorSet m_SliceImGuiTexture = VK_NULL_HANDLE; // panel preview
		bool m_SliceInitialized = false;
		bool m_ParticlesReady = false;
		bool m_ShellReady = false;
		uint32_t m_FrameCounter = 0;

		VkDescriptorSetLayout m_SetLayout = VK_NULL_HANDLE;
		std::array<VkDescriptorSet, 2> m_DescriptorSets{};

		VulkanComputePipeline m_ClearMaskPipeline;
		VulkanComputePipeline m_VoxelizePipeline;
		VulkanComputePipeline m_FloodSweepPipeline;
		VulkanComputePipeline m_FloodFinalizePipeline;
		VulkanComputePipeline m_InitializePipeline;
		VulkanComputePipeline m_ProjectedAreaPipeline;
		VulkanComputePipeline m_StreamCollidePipeline;
		VulkanComputePipeline m_ReducePartialPipeline;
		VulkanComputePipeline m_ReduceFinalPipeline;
		VulkanComputePipeline m_SlicePipeline;
		VulkanComputePipeline m_InitializeParticlesPipeline;
		VulkanComputePipeline m_AdvectParticlesPipeline;
		VulkanComputePipeline m_ShellPipeline;
		VulkanComputePipeline m_ShellFinalizePipeline;

		uint32_t m_CurrentSet = 0;
		uint32_t m_PendingSteps = 0;
		bool m_ResetRequested = true;
		uint64_t m_StepCount = 0;

		glm::vec3 m_LatticeForce{ 0.0f };
		glm::vec3 m_LatticeTorque{ 0.0f };
		uint32_t m_ProjectedCells = 0;
		uint32_t m_FloodResidual = 0;
		uint32_t m_RejectedTriangles = 0;
		uint32_t m_SolidCells = 0;
	};
}