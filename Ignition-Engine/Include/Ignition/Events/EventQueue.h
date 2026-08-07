#pragma once

#include "Ignition/Core/Log.h"
#include "Ignition/Events/Event.h"

#include <memory>
#include <utility>
#include <vector>

namespace Ignition
{
	class EventQueue
	{
	public:
		static constexpr size_t c_MaximumQueuedEvents = 4096;

		template<typename T, typename... Args>
		void Push(Args&&... args)
		{
			if (m_Events.size() >= c_MaximumQueuedEvents)
			{
				IG_CORE_WARN("Event queue full ({} events), dropping event", c_MaximumQueuedEvents);

				return;
			}

			m_Events.emplace_back(std::make_unique<T>(std::forward<Args>(args)...));
		}

		void Clear() { m_Events.clear(); }

		std::vector<std::unique_ptr<Event>> Take() { return std::exchange(m_Events, {}); }

		bool IsEmpty() const { return m_Events.empty(); }
		size_t GetCount() const { return m_Events.size(); }

		auto begin() { return m_Events.begin(); }
		auto end() { return m_Events.end(); }

	private:
		std::vector<std::unique_ptr<Event>> m_Events;
	};
}