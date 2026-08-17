#include "Ignition/Renderer/Vulkan/VulkanFluidSolver2D.h"

#include "Ignition/Renderer/Vulkan/VulkanDescriptorAllocator.h"
#include "Ignition/Renderer/Vulkan/VulkanGPUTimer.h"
#include "Ignition/Renderer/Vulkan/VulkanImage.h"
#include "Ignition/Renderer/Vulkan/VulkanRenderer.h"
#include "Ignition/Renderer/Vulkan/VulkanShaderTypes.h"
#include "Ignition/Renderer/Vulkan/Utilities/VulkanUtilities.h"
#include "Ignition/Core/Log.h"

#include <array>
#include <cstring>
#include <vector>

namespace Ignition
{
	namespace
	{
		constexpr uint32_t DirectionCount = 9;
		constexpr uint32_t WorkgroupSize = 8;
		constexpr uint32_t ReductionWorkgroupSize = 256;
		constexpr VkFormat VisualizationFormat = VK_FORMAT_R8G8B8A8_UNORM;

		uint32_t GroupCount(uint32_t threads, uint32_t groupSize)
		{
			return (threads + groupSize - 1) / groupSize;
		}
	}

	VulkanFluidSolver2D::VulkanFluidSolver2D() = default;
	VulkanFluidSolver2D::~VulkanFluidSolver2D() = default;

	void VulkanFluidSolver2D::Initialize(VulkanRenderer& renderer, VkDevice device, VmaAllocator allocator, VulkanDescriptorAllocator& descriptorAllocator, const std::string& spirvPath, const FluidSolver2DSettings& settings)
	{
		IG_CORE_INFO("------- INITIALIZING FLUID SOLVER ({}x{}) -------", settings.Width, settings.Height);

		m_Device = device;
		m_Allocator = allocator;
		m_DescriptorAllocator = &descriptorAllocator;
		m_Renderer = renderer.GetSelfReference();
		m_Settings = settings;
		m_Scaling = FluidSolver2D::ComputeScaling(settings);

		if (settings.Width < 16 || settings.Height < 16)
		{
			IG_CORE_ERROR("Fluid solver initialization aborted: the lattice must be at least 16x16");

			return;
		}

		if (!CreateBuffers() || !CreateDescriptors() || !CreatePipelines(spirvPath))
		{
			Shutdown();

			return;
		}

		m_ImGuiTexture = renderer.AddImGuiTexture(m_Visualization->GetImageView());
		m_ResetRequested = true;

		IG_CORE_INFO("------- FLUID SOLVER INITIALIZED -------");
	}

	bool VulkanFluidSolver2D::CreateBuffers()
	{
		const VkDeviceSize cells = static_cast<VkDeviceSize>(m_Settings.Width) * m_Settings.Height;

		for (VulkanBuffer& distribution : m_Distributions)
		{
			distribution.Initialize(m_Allocator, cells * DirectionCount * sizeof(float), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
		}

		m_Flags.Initialize(m_Allocator, cells * sizeof(uint32_t), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
		m_Fields.Initialize(m_Allocator, cells * sizeof(float) * 4, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
		m_CellForces.Initialize(m_Allocator, cells * sizeof(float) * 2, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
		m_ForcePartials.Initialize(m_Allocator, ReductionGroups * sizeof(float) * 4, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
		m_ForceResult.Initialize(m_Allocator, sizeof(float) * 4, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

		for (VulkanBuffer& readback : m_ForceReadback)
		{
			readback.Initialize(m_Allocator, sizeof(float) * 4, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VulkanBufferAccess::HostRead);
		}

		m_Visualization = std::make_unique<VulkanImage>();
		m_Visualization->Initialize(m_Device, m_Allocator, m_Settings.Width, m_Settings.Height, VisualizationFormat, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

		const bool valid = m_Distributions[0].IsValid() && m_Distributions[1].IsValid() && m_Flags.IsValid() && m_Fields.IsValid()
			&& m_CellForces.IsValid() && m_ForcePartials.IsValid() && m_ForceResult.IsValid() && m_Visualization->IsValid();

		if (!valid)
		{
			IG_CORE_ERROR("Fluid solver initialization aborted: buffer or image allocation failed");
		}

		return valid;
	}

	bool VulkanFluidSolver2D::CreateDescriptors()
	{
		std::vector<VkDescriptorSetLayoutBinding> bindings(8);

		for (uint32_t binding = 0; binding < bindings.size(); ++binding)
		{
			bindings[binding].binding = binding;
			bindings[binding].descriptorType = binding == 7 ? VK_DESCRIPTOR_TYPE_STORAGE_IMAGE : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
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
				IG_CORE_ERROR("Fluid solver initialization aborted: descriptor set allocation failed");

				return false;
			}

			// The two sets differ only in which distribution buffer reads and which writes - that is the ping-pong
			const VulkanBuffer& source = m_Distributions[set];
			const VulkanBuffer& destination = m_Distributions[1 - set];

			VulkanDescriptorAllocator::WriteStorageBuffer(m_Device, m_DescriptorSets[set], 0, source.GetBuffer(), source.GetSize());
			VulkanDescriptorAllocator::WriteStorageBuffer(m_Device, m_DescriptorSets[set], 1, destination.GetBuffer(), destination.GetSize());
			VulkanDescriptorAllocator::WriteStorageBuffer(m_Device, m_DescriptorSets[set], 2, m_Flags.GetBuffer(), m_Flags.GetSize());
			VulkanDescriptorAllocator::WriteStorageBuffer(m_Device, m_DescriptorSets[set], 3, m_Fields.GetBuffer(), m_Fields.GetSize());
			VulkanDescriptorAllocator::WriteStorageBuffer(m_Device, m_DescriptorSets[set], 4, m_CellForces.GetBuffer(), m_CellForces.GetSize());
			VulkanDescriptorAllocator::WriteStorageBuffer(m_Device, m_DescriptorSets[set], 5, m_ForcePartials.GetBuffer(), m_ForcePartials.GetSize());
			VulkanDescriptorAllocator::WriteStorageBuffer(m_Device, m_DescriptorSets[set], 6, m_ForceResult.GetBuffer(), m_ForceResult.GetSize());
			VulkanDescriptorAllocator::WriteStorageImage(m_Device, m_DescriptorSets[set], 7, m_Visualization->GetImageView());
		}

		return true;
	}

	bool VulkanFluidSolver2D::CreatePipelines(const std::string& spirvPath)
	{
		const VkShaderModule shaderModule = Utilities::VulkanUtilities::CreateShaderModule(m_Device, spirvPath);

		if (shaderModule == VK_NULL_HANDLE)
		{
			IG_CORE_ERROR("Fluid solver disabled: could not load '{}'", spirvPath);

			return false;
		}

		constexpr uint32_t pushConstantSize = static_cast<uint32_t>(sizeof(FluidPushConstants));

		m_InitializePipeline.Initialize(m_Device, shaderModule, "initialize", m_SetLayout, pushConstantSize);
		m_StreamCollidePipeline.Initialize(m_Device, shaderModule, "streamCollide", m_SetLayout, pushConstantSize);
		m_VisualizePipeline.Initialize(m_Device, shaderModule, "visualize", m_SetLayout, pushConstantSize);
		m_ReducePartialPipeline.Initialize(m_Device, shaderModule, "reduceForcePartial", m_SetLayout, pushConstantSize);
		m_ReduceFinalPipeline.Initialize(m_Device, shaderModule, "reduceForceFinal", m_SetLayout, pushConstantSize);

		vkDestroyShaderModule(m_Device, shaderModule, nullptr);

		return IsValid();
	}

	bool VulkanFluidSolver2D::IsValid() const
	{
		return m_InitializePipeline.IsValid() && m_StreamCollidePipeline.IsValid() && m_VisualizePipeline.IsValid() && m_ReducePartialPipeline.IsValid() && m_ReduceFinalPipeline.IsValid() && m_Visualization && m_Visualization->IsValid();
	}

	void VulkanFluidSolver2D::Shutdown()
	{
		if (m_ImGuiTexture != VK_NULL_HANDLE)
		{
			// The renderer may already be gone if the solver outlived it, which is what the weak reference is for
			const std::shared_ptr<VulkanRenderer*> renderer = m_Renderer.lock();

			if (renderer && *renderer)
			{
				(*renderer)->RemoveImGuiTexture(m_ImGuiTexture);
			}

			m_ImGuiTexture = VK_NULL_HANDLE;
		}

		m_ReduceFinalPipeline.Shutdown();
		m_ReducePartialPipeline.Shutdown();
		m_VisualizePipeline.Shutdown();
		m_StreamCollidePipeline.Shutdown();
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

		if (m_Visualization)
		{
			m_Visualization->Shutdown();
			m_Visualization.reset();
		}

		for (VulkanBuffer& readback : m_ForceReadback)
		{
			readback.Shutdown();
		}

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

	void VulkanFluidSolver2D::Configure(const FluidSolver2DSettings& settings)
	{
		// A moved or resized obstacle changes the mask, which only the initialization kernel writes
		const bool obstacleChanged = settings.ObstacleDiameter != m_Settings.ObstacleDiameter || settings.ObstacleCenter != m_Settings.ObstacleCenter || settings.DomainHeight != m_Settings.DomainHeight;

		m_Settings = settings;
		m_Scaling = FluidSolver2D::ComputeScaling(settings);

		if (obstacleChanged)
		{
			m_ResetRequested = true;
		}
	}

	void VulkanFluidSolver2D::DispatchGrid(VkCommandBuffer commandBuffer, const VulkanComputePipeline& pipeline, VkDescriptorSet descriptorSet) const
	{
		FluidPushConstants parameters{};
		parameters.Resolution = { m_Settings.Width, m_Settings.Height };
		parameters.ObstacleCenter = { m_Settings.ObstacleCenter.x * static_cast<float>(m_Settings.Width), m_Settings.ObstacleCenter.y * static_cast<float>(m_Settings.Height) };
		parameters.ObstacleRadius = 0.5f * m_Scaling.ObstacleDiameterCells;
		parameters.LatticeVelocity = m_Scaling.LatticeVelocity;
		parameters.RelaxationTime = m_Scaling.RelaxationTime;
		parameters.SmagorinskyConstant = m_Settings.SmagorinskyConstant;
		parameters.ReductionGroups = ReductionGroups;
		parameters.Field = static_cast<uint32_t>(m_Field);
		parameters.ColorScale = m_ColorScale;

		pipeline.Bind(commandBuffer);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.GetPipelineLayout(), 0, 1, &descriptorSet, 0, nullptr);
		vkCmdPushConstants(commandBuffer, pipeline.GetPipelineLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(FluidPushConstants), &parameters);

		pipeline.Dispatch(commandBuffer, GroupCount(m_Settings.Width, WorkgroupSize), GroupCount(m_Settings.Height, WorkgroupSize));
	}

	void VulkanFluidSolver2D::DispatchLinear(VkCommandBuffer commandBuffer, const VulkanComputePipeline& pipeline, VkDescriptorSet descriptorSet, uint32_t groups) const
	{
		FluidPushConstants parameters{};
		parameters.Resolution = { m_Settings.Width, m_Settings.Height };
		parameters.ReductionGroups = ReductionGroups;

		pipeline.Bind(commandBuffer);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.GetPipelineLayout(), 0, 1, &descriptorSet, 0, nullptr);
		vkCmdPushConstants(commandBuffer, pipeline.GetPipelineLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(FluidPushConstants), &parameters);

		pipeline.Dispatch(commandBuffer, groups);
	}

	void VulkanFluidSolver2D::TransitionVisualization(VkCommandBuffer commandBuffer, bool toStorage)
	{
		if (toStorage)
		{
			const VkImageLayout oldLayout = m_VisualizationInitialized ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;

			Utilities::VulkanUtilities::TransitionImageLayout(commandBuffer, m_Visualization->GetImage(), VK_IMAGE_ASPECT_COLOR_BIT, oldLayout, VK_IMAGE_LAYOUT_GENERAL,
				VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT);

			return;
		}

		Utilities::VulkanUtilities::TransitionImageLayout(commandBuffer, m_Visualization->GetImage(), VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT);

		m_VisualizationInitialized = true;
	}

	void VulkanFluidSolver2D::ReadForce(uint32_t frameIndex)
	{
		if (!m_ForceReadbackPending[frameIndex])
		{
			return;
		}

		VulkanBuffer& readback = m_ForceReadback[frameIndex];

		readback.Invalidate();

		if (void* mapped = readback.Map())
		{
			std::array<float, 4> values{};
			std::memcpy(values.data(), mapped, sizeof(values));

			m_LatticeForce = { values[0], values[1] };

			readback.Unmap();
		}

		m_ForceReadbackPending[frameIndex] = false;
	}

	void VulkanFluidSolver2D::RecordCompute(VkCommandBuffer commandBuffer, uint32_t frameIndex, VulkanGPUTimer* timer)
	{
		if (!IsValid())
		{
			return;
		}

		// The fence for this frame index was waited on before the command buffer opened, so last cycle's result is here
		ReadForce(frameIndex);

		const uint32_t simulationPass = timer ? timer->BeginPass(commandBuffer, "Fluid Sim") : UINT32_MAX;

		if (m_ResetRequested)
		{
			// Both distribution buffers are filled so the first step reads a consistent state whichever set it binds
			DispatchGrid(commandBuffer, m_InitializePipeline, m_DescriptorSets[0]);
			Utilities::VulkanUtilities::ComputeToComputeBarrier(commandBuffer);
			DispatchGrid(commandBuffer, m_InitializePipeline, m_DescriptorSets[1]);
			Utilities::VulkanUtilities::ComputeToComputeBarrier(commandBuffer);

			m_CurrentSet = 0;
			m_StepCount = 0;
			m_LatticeForce = glm::vec2(0.0f);
			m_ResetRequested = false;
		}

		const uint32_t steps = m_PendingSteps;
		m_PendingSteps = 0;

		for (uint32_t step = 0; step < steps; ++step)
		{
			DispatchGrid(commandBuffer, m_StreamCollidePipeline, m_DescriptorSets[m_CurrentSet]);
			Utilities::VulkanUtilities::ComputeToComputeBarrier(commandBuffer);

			m_CurrentSet ^= 1u;
			++m_StepCount;
		}

		if (steps > 0)
		{
			// Momentum exchange is summed once per frame: the reported force is the last simulated step's
			DispatchLinear(commandBuffer, m_ReducePartialPipeline, m_DescriptorSets[m_CurrentSet], ReductionGroups);
			Utilities::VulkanUtilities::ComputeToComputeBarrier(commandBuffer);
			DispatchLinear(commandBuffer, m_ReduceFinalPipeline, m_DescriptorSets[m_CurrentSet], 1);

			Utilities::VulkanUtilities::BufferBarrier(commandBuffer, m_ForceResult.GetBuffer(), 0, VK_WHOLE_SIZE,
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT, VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_READ_BIT);

			VkBufferCopy copyRegion{};
			copyRegion.size = sizeof(float) * 4;

			vkCmdCopyBuffer(commandBuffer, m_ForceResult.GetBuffer(), m_ForceReadback[frameIndex].GetBuffer(), 1, &copyRegion);

			m_ForceReadbackPending[frameIndex] = true;
		}

		if (timer)
		{
			timer->EndPass(commandBuffer, simulationPass);
		}

		// Visualization runs every frame, not only when stepping: the field selector stays live while paused, and the image is guaranteed to be in a layout ImGui can sample before the composite pass
		const uint32_t visualizationPass = timer ? timer->BeginPass(commandBuffer, "Fluid Viz") : UINT32_MAX;

		TransitionVisualization(commandBuffer, true);
		DispatchGrid(commandBuffer, m_VisualizePipeline, m_DescriptorSets[m_CurrentSet]);
		TransitionVisualization(commandBuffer, false);

		if (timer)
		{
			timer->EndPass(commandBuffer, visualizationPass);
		}
	}
}