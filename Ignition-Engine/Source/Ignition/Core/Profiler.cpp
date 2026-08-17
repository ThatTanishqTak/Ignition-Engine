#include "Ignition/Core/Profiler.h"

#if defined(IGNITION_PROFILE)
#include <tracy/Tracy.hpp>
#include <tracy/TracyC.h>
#endif

#if defined(IGNITION_PROFILE) && !defined(TRACY_ENABLE)
#error "IGNITION_PROFILE is set but TRACY_ENABLE is not. TracyClient did not propagate its definitions - check that Vendor/CMakeLists sets TRACY_ENABLE before add_subdirectory(tracy), and that Ignition links TracyClient."
#endif

#include <cstring>

namespace Ignition
{
#if defined(TRACY_ENABLE)
	namespace
	{
		uint64_t PackZone(TracyCZoneCtx context)
		{
			return (static_cast<uint64_t>(context.active ? 1u : 0u) << 32) | static_cast<uint64_t>(context.id);
		}

		TracyCZoneCtx UnpackZone(uint64_t zone)
		{
			TracyCZoneCtx context{};
			context.id = static_cast<uint32_t>(zone & 0xFFFFFFFFull);
			context.active = static_cast<int>((zone >> 32) & 1ull);

			return context;
		}
	}
#endif

	void Profiler::Initialize()
	{
#if defined(TRACY_ENABLE)
		tracy::StartupProfiler();
#endif
	}

	void Profiler::Shutdown()
	{
#if defined(TRACY_ENABLE)
		tracy::ShutdownProfiler();
#endif
	}

	bool Profiler::IsConnected()
	{
#if defined(TRACY_ENABLE)
		return TracyIsConnected;
#else
		return false;
#endif
	}

	void Profiler::MarkFrame()
	{
		FrameMark;
	}

	void Profiler::Plot(const char* name, float value)
	{
		TracyPlot(name, value);
	}

	void Profiler::Message(std::string_view message)
	{
		TracyMessage(message.data(), message.size());
	}

	ProfileScope::ProfileScope(const char* name, const char* file, int line, const char* function)
	{
#if defined(TRACY_ENABLE)
		static_assert(sizeof(tracy::ScopedZone) <= sizeof(m_Storage), "ProfileScope storage is too small for tracy::ScopedZone");
		static_assert(alignof(tracy::ScopedZone) <= 16, "ProfileScope storage is under-aligned for tracy::ScopedZone");

		new (m_Storage) tracy::ScopedZone(static_cast<uint32_t>(line), file, std::strlen(file), function, std::strlen(function), name, std::strlen(name), true);
#else
		(void)name;
		(void)file;
		(void)line;
		(void)function;
#endif
	}

	ProfileScope::~ProfileScope()
	{
#if defined(TRACY_ENABLE)
		std::launder(reinterpret_cast<tracy::ScopedZone*>(m_Storage))->~ScopedZone();
#endif
	}

	void ProfileScope::Annotate(std::string_view text) const
	{
#if defined(TRACY_ENABLE)
		std::launder(reinterpret_cast<tracy::ScopedZone*>(m_Storage))->Text(text.data(), text.size());
#else
		(void)text;
#endif
	}
}