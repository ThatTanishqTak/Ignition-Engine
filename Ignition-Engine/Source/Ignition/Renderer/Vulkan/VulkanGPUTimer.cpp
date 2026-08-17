#include "Ignition/Renderer/Vulkan/VulkanGPUTimer.h"

#include "Ignition/Renderer/Vulkan/Utilities/VulkanUtilities.h"
#include "Ignition/Core/Log.h"

namespace Ignition
{
	VulkanGPUTimer::VulkanGPUTimer() = default;
	VulkanGPUTimer::~VulkanGPUTimer() = default;

	void VulkanGPUTimer::Initialize(VkDevice device, float timestampPeriod)
	{
		m_Device = device;
		m_TimestampPeriod = timestampPeriod;

		if (device == VK_NULL_HANDLE || timestampPeriod <= 0.0f)
		{
			IG_CORE_WARN("GPU timing unavailable: the device reports no timestamp support");

			return;
		}

		VkQueryPoolCreateInfo poolCreateInfo{};
		poolCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
		poolCreateInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
		poolCreateInfo.queryCount = MaximumPasses * 2;

		for (uint32_t frame = 0; frame < VulkanFrameContext::MaximumFramesInFlight; ++frame)
		{
			if (!VK_CHECK(vkCreateQueryPool(m_Device, &poolCreateInfo, nullptr, &m_QueryPools[frame])))
			{
				m_QueryPools[frame] = VK_NULL_HANDLE;

				Shutdown();

				return;
			}

			m_PassNames[frame].reserve(MaximumPasses);
		}
	}

	void VulkanGPUTimer::Shutdown()
	{
		for (VkQueryPool& pool : m_QueryPools)
		{
			if (pool != VK_NULL_HANDLE)
			{
				vkDestroyQueryPool(m_Device, pool, nullptr);
				pool = VK_NULL_HANDLE;
			}
		}

		m_Device = VK_NULL_HANDLE;
		m_TimestampPeriod = 0.0f;
		m_Results.clear();
	}

	void VulkanGPUTimer::BeginFrame(VkCommandBuffer commandBuffer, uint32_t frameIndex)
	{
		if (!IsValid())
		{
			return;
		}

		m_FrameIndex = frameIndex;

		const VkQueryPool pool = m_QueryPools[frameIndex];
		const std::vector<std::string>& names = m_PassNames[frameIndex];

		if (m_Recorded[frameIndex] && !names.empty())
		{
			// The caller waited on this frame's fence, so every timestamp it wrote has landed
			std::array<uint64_t, MaximumPasses * 2> values{};
			const uint32_t queryCount = static_cast<uint32_t>(names.size()) * 2;

			const VkResult result = vkGetQueryPoolResults(m_Device, pool, 0, queryCount, sizeof(uint64_t) * queryCount, values.data(), sizeof(uint64_t), VK_QUERY_RESULT_64_BIT);

			if (result == VK_SUCCESS)
			{
				m_Results.clear();

				for (size_t pass = 0; pass < names.size(); ++pass)
				{
					const uint64_t elapsed = values[pass * 2 + 1] - values[pass * 2];

					m_Results.push_back({ names[pass], static_cast<float>(elapsed) * m_TimestampPeriod * 1e-6f });
				}
			}
		}

		vkCmdResetQueryPool(commandBuffer, pool, 0, MaximumPasses * 2);

		m_PassNames[frameIndex].clear();
		m_Recorded[frameIndex] = false;
	}

	uint32_t VulkanGPUTimer::BeginPass(VkCommandBuffer commandBuffer, const char* name)
	{
		if (!IsValid() || m_PassNames[m_FrameIndex].size() >= MaximumPasses)
		{
			return UINT32_MAX;
		}

		const uint32_t pass = static_cast<uint32_t>(m_PassNames[m_FrameIndex].size());
		m_PassNames[m_FrameIndex].emplace_back(name ? name : "Pass");

		vkCmdWriteTimestamp2(commandBuffer, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, m_QueryPools[m_FrameIndex], pass * 2);

		return pass;
	}

	void VulkanGPUTimer::EndPass(VkCommandBuffer commandBuffer, uint32_t pass)
	{
		if (!IsValid() || pass == UINT32_MAX || pass >= m_PassNames[m_FrameIndex].size())
		{
			return;
		}

		vkCmdWriteTimestamp2(commandBuffer, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, m_QueryPools[m_FrameIndex], pass * 2 + 1);

		m_Recorded[m_FrameIndex] = true;
	}
}