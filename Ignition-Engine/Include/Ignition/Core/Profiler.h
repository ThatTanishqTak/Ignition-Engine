#pragma once

#include "Ignition/Core/Export.h"

#include <cstdint>
#include <string_view>

namespace Ignition
{
	class Profiler
	{
	public:
		IGNITION_API static void Initialize();
		IGNITION_API static void Shutdown();

		IGNITION_API static bool IsConnected();

		IGNITION_API static void MarkFrame();

		// `name` must outlive the process - pass a string literal, Tracy stores the pointer
		IGNITION_API static void Plot(const char* name, float value);
		IGNITION_API static void Message(std::string_view message);

		// Raw zone handles behind ProfileScope; prefer the macros
		IGNITION_API static uint64_t BeginZone(const char* name, const char* file, int line, const char* function);
		IGNITION_API static void EndZone(uint64_t zone);
		IGNITION_API static void AnnotateZone(uint64_t zone, std::string_view text);
	};

	class ProfileScope
	{
	public:
		ProfileScope(const char* name, const char* file, int line, const char* function) : m_Zone(Profiler::BeginZone(name, file, line, function)) {}
		~ProfileScope() { Profiler::EndZone(m_Zone); }

		ProfileScope(const ProfileScope&) = delete;
		ProfileScope& operator=(const ProfileScope&) = delete;

		void Annotate(std::string_view text) const { Profiler::AnnotateZone(m_Zone, text); }

	private:
		uint64_t m_Zone = 0;
	};
}

#if defined(IGNITION_PROFILE)
#define IG_INTERNAL_PROFILE_CONCATENATE(a, b) a##b
#define IG_INTERNAL_PROFILE_NAME(line) IG_INTERNAL_PROFILE_CONCATENATE(ig_profileScope, line)

#define IG_PROFILE_SCOPE(name) const ::Ignition::ProfileScope IG_INTERNAL_PROFILE_NAME(__LINE__)(name, __FILE__, __LINE__, __func__)
#define IG_PROFILE_FUNCTION() IG_PROFILE_SCOPE(__func__)
#define IG_PROFILE_FRAME() ::Ignition::Profiler::MarkFrame()
#define IG_PROFILE_PLOT(name, value) ::Ignition::Profiler::Plot(name, value)
#else
#define IG_PROFILE_SCOPE(name) ((void)0)
#define IG_PROFILE_FUNCTION() ((void)0)
#define IG_PROFILE_FRAME() ((void)0)
#define IG_PROFILE_PLOT(name, value) ((void)0)
#endif