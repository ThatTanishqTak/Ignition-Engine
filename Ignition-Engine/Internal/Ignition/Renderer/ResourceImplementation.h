#pragma once

#include "Ignition/Renderer/Vulkan/VulkanRenderer.h"

#include <memory>
#include <utility>

namespace Ignition
{
	template <typename TBackendResource>
	struct ResourceImplementation
	{
		std::unique_ptr<TBackendResource> Handle;
		std::weak_ptr<VulkanRenderer*> Backend;

		~ResourceImplementation()
		{
			if (!Handle)
			{
				return;
			}

			const std::shared_ptr<VulkanRenderer*> renderer = Backend.lock();

			if (renderer && *renderer)
			{
				(*renderer)->Retire(std::move(Handle));
			}
			else
			{
				Handle->Shutdown();
			}
		}
	};
}