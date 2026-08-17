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

	uint64_t Profiler::BeginZone(const char* name, const char* file, int line, const char* function)
	{
#if defined(TRACY_ENABLE)
		const uint64_t sourceLocation = ___tracy_alloc_srcloc_name(static_cast<uint32_t>(line), file, std::strlen(file), function, std::strlen(function), name, std::strlen(name), 0);

		return PackZone(___tracy_emit_zone_begin_alloc(sourceLocation, 1));
#else
		(void)name;
		(void)file;
		(void)line;
		(void)function;

		return 0;
#endif
	}

	void Profiler::EndZone(uint64_t zone)
	{
#if defined(TRACY_ENABLE)
		___tracy_emit_zone_end(UnpackZone(zone));
#else
		(void)zone;
#endif
	}

	void Profiler::AnnotateZone(uint64_t zone, std::string_view text)
	{
#if defined(TRACY_ENABLE)
		___tracy_emit_zone_text(UnpackZone(zone), text.data(), text.size());
#else
		(void)zone;
		(void)text;
#endif
	}
}