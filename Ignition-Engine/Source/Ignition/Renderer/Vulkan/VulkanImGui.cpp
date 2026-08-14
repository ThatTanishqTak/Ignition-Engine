#include "Ignition/Renderer/Vulkan/VulkanImGui.h"

#include "Ignition/Renderer/Vulkan/Utilities/VulkanUtilities.h"
#include "Ignition/Core/Log.h"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>

#include <SDL3/SDL_events.h>

#include <array>

namespace Ignition
{
	namespace
	{
		float SRGBToLinear(float channel)
		{
			return channel <= 0.04045f ? channel / 12.92f : std::pow((channel + 0.055f) / 1.055f, 2.4f);
		}
	}

	VulkanImGui::VulkanImGui() = default;
	VulkanImGui::~VulkanImGui() = default;

	void VulkanImGui::Initialize(SDL_Window* window, VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, uint32_t graphicsQueueFamily, VkQueue graphicsQueue, uint32_t swapchainImageCount, VkFormat colorFormat, VkFormat depthFormat)
	{
		IG_CORE_INFO("------- INITIALIZING IMGUI -------");

		m_Device = device;
		m_ColorFormat = colorFormat;

		const std::array<VkDescriptorPoolSize, 3> poolSizes = { {
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 16 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 16 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 16 },
		} };

		VkDescriptorPoolCreateInfo poolCreateInfo{};
		poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		poolCreateInfo.maxSets = 48;
		poolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolCreateInfo.pPoolSizes = poolSizes.data();

		if (!VK_CHECK(vkCreateDescriptorPool(m_Device, &poolCreateInfo, nullptr, &m_DescriptorPool)))
		{
			m_DescriptorPool = VK_NULL_HANDLE;

			return;
		}

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

		ImGui::StyleColorsDark();

		ImGuiStyle& style = ImGui::GetStyle();

		for (int i = 0; i < ImGuiCol_COUNT; i++)
		{
			ImVec4& color = style.Colors[i];
			color.x = SRGBToLinear(color.x);
			color.y = SRGBToLinear(color.y);
			color.z = SRGBToLinear(color.z);
		}

		if (!ImGui_ImplSDL3_InitForVulkan(window))
		{
			IG_CORE_ERROR("ImGui SDL3 backend initialization failed");

			ImGui::DestroyContext();
			vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
			m_DescriptorPool = VK_NULL_HANDLE;

			return;
		}

		ImGui_ImplVulkan_InitInfo initInfo{};
		initInfo.ApiVersion = VK_API_VERSION_1_4;
		initInfo.Instance = instance;
		initInfo.PhysicalDevice = physicalDevice;
		initInfo.Device = device;
		initInfo.QueueFamily = graphicsQueueFamily;
		initInfo.Queue = graphicsQueue;
		initInfo.DescriptorPool = m_DescriptorPool;
		initInfo.MinImageCount = 2;
		initInfo.ImageCount = swapchainImageCount;
		initInfo.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		initInfo.UseDynamicRendering = true;
		initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
		initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
		initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &m_ColorFormat;
		initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.depthAttachmentFormat = depthFormat;

		if (!ImGui_ImplVulkan_Init(&initInfo))
		{
			IG_CORE_ERROR("ImGui Vulkan backend initialization failed");

			ImGui_ImplSDL3_Shutdown();
			ImGui::DestroyContext();
			vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
			m_DescriptorPool = VK_NULL_HANDLE;

			return;
		}

		m_Initialized = true;

		IG_CORE_INFO("------- IMGUI INITIALIZED -------");
	}

	void VulkanImGui::Shutdown()
	{
		if (!m_Initialized)
		{
			if (m_DescriptorPool != VK_NULL_HANDLE)
			{
				vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
				m_DescriptorPool = VK_NULL_HANDLE;
			}

			return;
		}

		IG_CORE_INFO("------- SHUTTING DOWN IMGUI -------");

		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplSDL3_Shutdown();
		ImGui::DestroyContext();

		vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
		m_DescriptorPool = VK_NULL_HANDLE;

		m_Initialized = false;

		IG_CORE_INFO("------- IMGUI SHUTDOWN COMPLETE -------");
	}

	void VulkanImGui::ProcessEvent(const void* sdlEvent)
	{
		if (m_Initialized && sdlEvent)
		{
			ImGui_ImplSDL3_ProcessEvent(static_cast<const SDL_Event*>(sdlEvent));
		}
	}

	void VulkanImGui::BeginFrame()
	{
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();
	}

	void VulkanImGui::Render(VkCommandBuffer commandBuffer)
	{
		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
	}

	VkDescriptorSet VulkanImGui::AddTexture(VkSampler sampler, VkImageView imageView)
	{
		if (!m_Initialized)
		{
			return VK_NULL_HANDLE;
		}

		return ImGui_ImplVulkan_AddTexture(sampler, imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	void VulkanImGui::RemoveTexture(VkDescriptorSet descriptorSet)
	{
		if (m_Initialized && descriptorSet != VK_NULL_HANDLE)
		{
			ImGui_ImplVulkan_RemoveTexture(descriptorSet);
		}
	}

	bool VulkanImGui::WantCaptureMouse() const
	{
		return m_Initialized && ImGui::GetIO().WantCaptureMouse;
	}

	bool VulkanImGui::WantCaptureKeyboard() const
	{
		return m_Initialized && ImGui::GetIO().WantCaptureKeyboard;
	}
}