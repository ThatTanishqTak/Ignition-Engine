#include "Ignition/Renderer/Vulkan/VulkanFluidSolver3D.h"

#include "Ignition/Renderer/Vulkan/VulkanDescriptorAllocator.h"
#include "Ignition/Renderer/Vulkan/VulkanGPUTimer.h"
#include "Ignition/Renderer/Vulkan/VulkanShaderTypes.h"
#include "Ignition/Renderer/Vulkan/Utilities/VulkanUtilities.h"
#include "Ignition/Core/Log.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <vector>

namespace Ignition
{
	namespace
	{
		constexpr uint32_t DirectionCount = 19;
		constexpr uint32_t WorkgroupSizeXY = 8;
		constexpr uint32_t WorkgroupSizeZ = 4;
		constexpr uint32_t ReductionWorkgroupSize = 256;

		// Six floats of force and torque, then the projected column count as a uint
		constexpr VkDeviceSize ResultFloats = 6;
		constexpr VkDeviceSize ResultSize = 8 * sizeof(float);
		constexpr VkDeviceSize ReadbackSize = ResultFloats * sizeof(float) + sizeof(uint32_t) + sizeof(uint32_t);

		uint32_t GroupCount(uint32_t threads, uint32_t groupSize)
		{
			return (threads + groupSize - 1) / groupSize;
		}
	}

	VulkanFluidSolver3D::VulkanFluidSolver3D() = default;
	VulkanFluidSolver3D::~VulkanFluidSolver3D() = default;

	void VulkanFluidSolver3D::Initialize(VkDevice device, VmaAllocator allocator, VulkanDescriptorAllocator& descriptorAllocator, const std::string& spirvPath, const FluidSolver3DSettings& settings)
	{
		IG_CORE_INFO("------- INITIALIZING WIND TUNNEL ({}x{}x{}) -------", settings.Resolution.x, settings.Resolution.y, settings.Resolution.z);

		m_Device = device;
		m_Allocator = allocator;
		m_DescriptorAllocator = &descriptorAllocator;
		m_Settings = settings;
		m_Scaling = FluidSolver3D::ComputeScaling(settings);

		if (settings.Resolution.x < 16 || settings.Resolution.y < 16 || settings.Resolution.z < 16)
		{
			IG_CORE_ERROR("Wind tunnel initialization aborted: the lattice must be at least 16 cells on every axis");

			return;
		}

		if (!CreateBuffers() || !CreateDescriptors() || !CreatePipelines(spirvPath))
		{
			Shutdown();

			return;
		}

		m_ResetRequested = true;

		IG_CORE_INFO("------- WIND TUNNEL INITIALIZED -------");
	}

	bool VulkanFluidSolver3D::CreateBuffers()
	{
		const VkDeviceSize cells = static_cast<VkDeviceSize>(m_Settings.Resolution.x) * m_Settings.Resolution.y * m_Settings.Resolution.z;
		const VkDeviceSize latticeBytes = cells * DirectionCount * sizeof(float) * 2;

		IG_CORE_INFO("Wind tunnel lattice: {} cells, {:.0f} MB of distributions", cells, static_cast<double>(latticeBytes) / (1024.0 * 1024.0));

		for (VulkanBuffer& distribution : m_Distributions)
		{
			distribution.Initialize(m_Allocator, cells * DirectionCount * sizeof(float), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
		}

		m_Flags.Initialize(m_Allocator, cells * sizeof(uint32_t), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
		m_Fields.Initialize(m_Allocator, cells * sizeof(float) * 4, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
		m_CellForces.Initialize(m_Allocator, cells * sizeof(float) * 3, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
		m_ForcePartials.Initialize(m_Allocator, ReductionGroups * sizeof(float) * 6, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

		// Cleared with vkCmdFillBuffer on reset, so a frame that only refreshes the area never reports a stale force
		m_ForceResult.Initialize(m_Allocator, ResultSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
		m_Area.Initialize(m_Allocator, sizeof(uint32_t), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

		for (VulkanBuffer& readback : m_Readback)
		{
			readback.Initialize(m_Allocator, ReadbackSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VulkanBufferAccess::HostRead);
		}

		const bool valid = m_Distributions[0].IsValid() && m_Distributions[1].IsValid() && m_Flags.IsValid() && m_Fields.IsValid()
			&& m_CellForces.IsValid() && m_ForcePartials.IsValid() && m_ForceResult.IsValid() && m_Area.IsValid();

		if (!valid)
		{
			IG_CORE_ERROR("Wind tunnel initialization aborted: buffer allocation failed - check the lattice against the device's VRAM (Appendix B)");
		}

		return valid;
	}

	bool VulkanFluidSolver3D::CreateDescriptors()
	{
		std::vector<VkDescriptorSetLayoutBinding> bindings(8);

		for (uint32_t binding = 0; binding < bindings.size(); ++binding)
		{
			bindings[binding].binding = binding;
			bindings[binding].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			bindings[binding].descriptorCount = 1;
			bindings[binding].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		}

		m_SetLayout = m_DescriptorAllocator->CreateSetLayout(bindings);

		if (m_SetLayout == VK_NULL_HANDLE)
		{
			return false;
		}

		for (uint32_t set = 0; set < m_DescriptorSets.size(); ++set)
		{
			m_DescriptorSets[set] = m_DescriptorAllocator->Allocate(m_SetLayout);

			if (m_DescriptorSets[set] == VK_NULL_HANDLE)
			{
				IG_CORE_ERROR("Wind tunnel initialization aborted: descriptor set allocation failed");

				return false;
			}

			// The two sets differ only in which distribution buffer reads and which writes - that is the ping-pong, unchanged from 2D
			const VulkanBuffer& source = m_Distributions[set];
			const VulkanBuffer& destination = m_Distributions[1 - set];

			VulkanDescriptorAllocator::WriteStorageBuffer(m_Device, m_DescriptorSets[set], 0, source.GetBuffer(), source.GetSize());
			VulkanDescriptorAllocator::WriteStorageBuffer(m_Device, m_DescriptorSets[set], 1, destination.GetBuffer(), destination.GetSize());
			VulkanDescriptorAllocator::WriteStorageBuffer(m_Device, m_DescriptorSets[set], 2, m_Flags.GetBuffer(), m_Flags.GetSize());
			VulkanDescriptorAllocator::WriteStorageBuffer(m_Device, m_DescriptorSets[set], 3, m_Fields.GetBuffer(), m_Fields.GetSize());
			VulkanDescriptorAllocator::WriteStorageBuffer(m_Device, m_DescriptorSets[set], 4, m_CellForces.GetBuffer(), m_CellForces.GetSize());
			VulkanDescriptorAllocator::WriteStorageBuffer(m_Device, m_DescriptorSets[set], 5, m_ForcePartials.GetBuffer(), m_ForcePartials.GetSize());
			VulkanDescriptorAllocator::WriteStorageBuffer(m_Device, m_DescriptorSets[set], 6, m_ForceResult.GetBuffer(), m_ForceResult.GetSize());
			VulkanDescriptorAllocator::WriteStorageBuffer(m_Device, m_DescriptorSets[set], 7, m_Area.GetBuffer(), m_Area.GetSize());
		}

		return true;
	}

	bool VulkanFluidSolver3D::CreatePipelines(const std::string& spirvPath)
	{
		const VkShaderModule shaderModule = Utilities::VulkanUtilities::CreateShaderModule(m_Device, spirvPath);

		if (shaderModule == VK_NULL_HANDLE)
		{
			IG_CORE_ERROR("Wind tunnel disabled: could not load '{}'", spirvPath);

			return false;
		}

		constexpr uint32_t pushConstantSize = static_cast<uint32_t>(sizeof(FluidPushConstants3D));

		m_InitializePipeline.Initialize(m_Device, shaderModule, "initialize", m_SetLayout, pushConstantSize);
		m_ProjectedAreaPipeline.Initialize(m_Device, shaderModule, "projectedArea", m_SetLayout, pushConstantSize);
		m_StreamCollidePipeline.Initialize(m_Device, shaderModule, "streamCollide", m_SetLayout, pushConstantSize);
		m_ReducePartialPipeline.Initialize(m_Device, shaderModule, "reduceForcePartial", m_SetLayout, pushConstantSize);
		m_ReduceFinalPipeline.Initialize(m_Device, shaderModule, "reduceForceFinal", m_SetLayout, pushConstantSize);

		vkDestroyShaderModule(m_Device, shaderModule, nullptr);

		return IsValid();
	}

	bool VulkanFluidSolver3D::IsValid() const
	{
		return m_InitializePipeline.IsValid() && m_ProjectedAreaPipeline.IsValid() && m_StreamCollidePipeline.IsValid() && m_ReducePartialPipeline.IsValid() && m_ReduceFinalPipeline.IsValid();
	}

	void VulkanFluidSolver3D::Shutdown()
	{
		m_ReduceFinalPipeline.Shutdown();
		m_ReducePartialPipeline.Shutdown();
		m_StreamCollidePipeline.Shutdown();
		m_ProjectedAreaPipeline.Shutdown();
		m_InitializePipeline.Shutdown();

		if (m_DescriptorAllocator)
		{
			for (VkDescriptorSet& descriptorSet : m_DescriptorSets)
			{
				m_DescriptorAllocator->Free(descriptorSet);
				descriptorSet = VK_NULL_HANDLE;
			}

			if (m_SetLayout != VK_NULL_HANDLE)
			{
				m_DescriptorAllocator->DestroySetLayout(m_SetLayout);
				m_SetLayout = VK_NULL_HANDLE;
			}
		}

		for (VulkanBuffer& readback : m_Readback)
		{
			readback.Shutdown();
		}

		m_Area.Shutdown();
		m_ForceResult.Shutdown();
		m_ForcePartials.Shutdown();
		m_CellForces.Shutdown();
		m_Fields.Shutdown();
		m_Flags.Shutdown();

		for (VulkanBuffer& distribution : m_Distributions)
		{
			distribution.Shutdown();
		}

		m_Device = VK_NULL_HANDLE;
		m_Allocator = VK_NULL_HANDLE;
		m_DescriptorAllocator = nullptr;
	}

	void VulkanFluidSolver3D::Configure(const FluidSolver3DSettings& settings)
	{
		// Only the mask-shaping inputs force a reset; the reference point, wind speed and rolling road all ride in on push constants
		const bool geometryChanged = settings.ObstacleDiameter != m_Settings.ObstacleDiameter || settings.ObstacleCenter != m_Settings.ObstacleCenter || settings.DomainSize != m_Settings.DomainSize;

		m_Settings = settings;
		m_Scaling = FluidSolver3D::ComputeScaling(settings);

		if (geometryChanged)
		{
			m_ResetRequested = true;
		}
	}

	FluidPushConstants3D VulkanFluidSolver3D::BuildParameters() const
	{
		FluidPushConstants3D parameters{};

		parameters.Resolution = m_Settings.Resolution;
		parameters.ReductionGroups = ReductionGroups;

		const float cellSize = std::max(m_Scaling.CellSize, 1e-6f);

		parameters.ObstacleCenter = m_Settings.ObstacleCenter * glm::vec3(m_Settings.Resolution);
		parameters.ObstacleRadius = 0.5f * m_Settings.ObstacleDiameter / cellSize;

		// The lattice, not the requested box, is the domain: cubic cells mean the covered volume is Resolution * dx, and the floor sits on the origin
		const glm::vec3 extent = glm::vec3(m_Settings.Resolution) * cellSize;
		const glm::vec3 minimum = m_Settings.Origin - glm::vec3(0.5f * extent.x, 0.0f, 0.5f * extent.z);

		parameters.ReferencePoint = (m_Settings.ReferencePoint - minimum) / cellSize;

		parameters.LatticeVelocity = m_Scaling.LatticeVelocity;
		parameters.RelaxationTime = m_Scaling.RelaxationTime;
		parameters.SmagorinskyConstant = m_Settings.SmagorinskyConstant;
		parameters.Flags = m_Settings.RollingRoad ? 1u : 0u;

		return parameters;
	}

	void VulkanFluidSolver3D::Dispatch(VkCommandBuffer commandBuffer, const VulkanComputePipeline& pipeline, VkDescriptorSet descriptorSet, uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ) const
	{
		const FluidPushConstants3D parameters = BuildParameters();

		pipeline.Bind(commandBuffer);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.GetPipelineLayout(), 0, 1, &descriptorSet, 0, nullptr);
		vkCmdPushConstants(commandBuffer, pipeline.GetPipelineLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(FluidPushConstants3D), &parameters);

		pipeline.Dispatch(commandBuffer, groupsX, groupsY, groupsZ);
	}

	void VulkanFluidSolver3D::CopyResults(VkCommandBuffer commandBuffer, uint32_t frameIndex)
	{
		Utilities::VulkanUtilities::MemoryBarrier(commandBuffer, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT, VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_READ_BIT);

		VkBufferCopy forceRegion{};
		forceRegion.size = ResultFloats * sizeof(float);

		VkBufferCopy areaRegion{};
		areaRegion.dstOffset = ResultFloats * sizeof(float);
		areaRegion.size = sizeof(uint32_t);

		vkCmdCopyBuffer(commandBuffer, m_ForceResult.GetBuffer(), m_Readback[frameIndex].GetBuffer(), 1, &forceRegion);
		vkCmdCopyBuffer(commandBuffer, m_Area.GetBuffer(), m_Readback[frameIndex].GetBuffer(), 1, &areaRegion);

		m_ReadbackPending[frameIndex] = true;
	}

	void VulkanFluidSolver3D::ReadResults(uint32_t frameIndex)
	{
		if (!m_ReadbackPending[frameIndex])
		{
			return;
		}

		VulkanBuffer& readback = m_Readback[frameIndex];

		readback.Invalidate();

		if (void* mapped = readback.Map())
		{
			std::array<float, ResultFloats> values{};
			uint32_t projected = 0;

			std::memcpy(values.data(), mapped, sizeof(values));
			std::memcpy(&projected, static_cast<const std::byte*>(mapped) + sizeof(values), sizeof(projected));

			m_LatticeForce = { values[0], values[1], values[2] };
			m_LatticeTorque = { values[3], values[4], values[5] };
			m_ProjectedCells = projected;

			readback.Unmap();
		}

		m_ReadbackPending[frameIndex] = false;
	}

	void VulkanFluidSolver3D::RecordCompute(VkCommandBuffer commandBuffer, uint32_t frameIndex, VulkanGPUTimer* timer)
	{
		if (!IsValid())
		{
			return;
		}

		// The fence for this frame index was waited on before the command buffer opened, so last cycle's result is here
		ReadResults(frameIndex);

		const uint32_t simulationPass = timer ? timer->BeginPass(commandBuffer, "Tunnel Sim") : UINT32_MAX;

		const uint32_t gridX = GroupCount(m_Settings.Resolution.x, WorkgroupSizeXY);
		const uint32_t gridY = GroupCount(m_Settings.Resolution.y, WorkgroupSizeXY);
		const uint32_t gridZ = GroupCount(m_Settings.Resolution.z, WorkgroupSizeZ);

		bool copyResults = false;

		if (m_ResetRequested)
		{
			// Zeroed here rather than left stale, so the frame that publishes a fresh reference area does not also publish an old force
			vkCmdFillBuffer(commandBuffer, m_ForceResult.GetBuffer(), 0, VK_WHOLE_SIZE, 0);

			Utilities::VulkanUtilities::BufferBarrier(commandBuffer, m_ForceResult.GetBuffer(), 0, VK_WHOLE_SIZE,
				VK_PIPELINE_STAGE_2_CLEAR_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_READ_BIT);

			// Both distribution buffers are filled so the first step reads a consistent state whichever set it binds
			Dispatch(commandBuffer, m_InitializePipeline, m_DescriptorSets[0], gridX, gridY, gridZ);
			Utilities::VulkanUtilities::ComputeToComputeBarrier(commandBuffer);
			Dispatch(commandBuffer, m_InitializePipeline, m_DescriptorSets[1], gridX, gridY, gridZ);
			Utilities::VulkanUtilities::ComputeToComputeBarrier(commandBuffer);

			// Frontal area is a property of the mask, so it is counted once per reset rather than every frame
			Dispatch(commandBuffer, m_ProjectedAreaPipeline, m_DescriptorSets[0], gridX, gridY, 1);
			Utilities::VulkanUtilities::ComputeToComputeBarrier(commandBuffer);

			m_CurrentSet = 0;
			m_StepCount = 0;
			m_LatticeForce = glm::vec3(0.0f);
			m_LatticeTorque = glm::vec3(0.0f);
			m_ResetRequested = false;

			copyResults = true;
		}

		const uint32_t steps = m_PendingSteps;
		m_PendingSteps = 0;

		for (uint32_t step = 0; step < steps; ++step)
		{
			Dispatch(commandBuffer, m_StreamCollidePipeline, m_DescriptorSets[m_CurrentSet], gridX, gridY, gridZ);
			Utilities::VulkanUtilities::ComputeToComputeBarrier(commandBuffer);

			m_CurrentSet ^= 1u;
			++m_StepCount;
		}

		if (steps > 0)
		{
			// Momentum exchange is summed once per frame: the reported force is the last simulated step's, as in 2D
			Dispatch(commandBuffer, m_ReducePartialPipeline, m_DescriptorSets[m_CurrentSet], ReductionGroups);
			Utilities::VulkanUtilities::ComputeToComputeBarrier(commandBuffer);
			Dispatch(commandBuffer, m_ReduceFinalPipeline, m_DescriptorSets[m_CurrentSet], 1);

			copyResults = true;
		}

		if (copyResults)
		{
			CopyResults(commandBuffer, frameIndex);
		}

		if (timer)
		{
			timer->EndPass(commandBuffer, simulationPass);
		}
	}
}