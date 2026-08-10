#include "Ignition/Renderer/Vulkan/VulkanImGui.h"

#include "Ignition/Renderer/Vulkan/Utilities/VulkanUtilities.h"
#include "Ignition/Core/Log.h"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>

#include <SDL3/SDL_events.h>

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

	void VulkanImGui::Initialize(SDL_Window* window, VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, uint32_t graphicsQueueFamily, VkQueue graphicsQueue, uint32_t swapchainImageCount, VkFormat colorFormat)
	{
		IG_CORE_INFO("------- INITIALIZING IMGUI -------");

		m_Device = device;
		m_ColorFormat = colorFormat;

		VkDescriptorPoolSize poolSize{};
		poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSize.descriptorCount = 16;

		VkDescriptorPoolCreateInfo poolCreateInfo{};
		poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		poolCreateInfo.maxSets = 16;
		poolCreateInfo.poolSizeCount = 1;
		poolCreateInfo.pPoolSizes = &poolSize;

		if (Utilities::VulkanUtilities::VKCheck(vkCreateDescriptorPool(m_Device, &poolCreateInfo, nullptr, &m_DescriptorPool), "Failed vkCreateDescriptorPool (ImGui)"))
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

	bool VulkanImGui::WantCaptureMouse() const
	{
		return m_Initialized && ImGui::GetIO().WantCaptureMouse;
	}

	bool VulkanImGui::WantCaptureKeyboard() const
	{
		return m_Initialized && ImGui::GetIO().WantCaptureKeyboard;
	}
}