#pragma once

#include "Ignition/Renderer/Renderer.h"
#include "Ignition/Renderer/Vulkan/VulkanFrameContext.h"

#include <vulkan/vulkan.h>

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace Ignition
{
	// Timestamp pairs around each pass, resolved one frame-in-flight later so nothing ever stalls the CPU.
	class VulkanGPUTimer
	{
	public:
		static constexpr uint32_t MaximumPasses = 8;

		VulkanGPUTimer();
		~VulkanGPUTimer();

		VulkanGPUTimer(const VulkanGPUTimer&) = delete;
		VulkanGPUTimer& operator=(const VulkanGPUTimer&) = delete;

		void Initialize(VkDevice device, float timestampPeriod);
		void Shutdown();

		bool IsValid() const { return m_TimestampPeriod > 0.0f && m_QueryPools[0] != VK_NULL_HANDLE; }

		// Collects the results this frame index recorded last time around, then resets its pool
		void BeginFrame(VkCommandBuffer commandBuffer, uint32_t frameIndex);

		uint32_t BeginPass(VkCommandBuffer commandBuffer, const char* name);
		void EndPass(VkCommandBuffer commandBuffer, uint32_t pass);

		const std::vector<PassTiming>& GetResults() const { return m_Results; }

	private:
		VkDevice m_Device = VK_NULL_HANDLE;
		float m_TimestampPeriod = 0.0f;
		uint32_t m_FrameIndex = 0;

		std::array<VkQueryPool, VulkanFrameContext::MaximumFramesInFlight> m_QueryPools{};
		std::array<std::vector<std::string>, VulkanFrameContext::MaximumFramesInFlight> m_PassNames;
		std::array<bool, VulkanFrameContext::MaximumFramesInFlight> m_Recorded{};

		std::vector<PassTiming> m_Results;
	};
}