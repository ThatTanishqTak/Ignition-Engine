#pragma once

#if defined(IGNITION_PROFILE)
#include <tracy/Tracy.hpp>
#endif

#if defined(IGNITION_PROFILE) && !defined(TRACY_ENABLE)
#error "IGNITION_PROFILE is set but TRACY_ENABLE is not. TracyClient did not propagate its definitions - check that Vendor/CMakeLists sets TRACY_ENABLE before add_subdirectory(tracy), and that Ignition links TracyClient."
#endif

#if defined(TRACY_ENABLE)
#define IG_PROFILE_ZONE() ZoneScoped
#define IG_PROFILE_ZONE_NAMED(name) ZoneScopedN(name)
#define IG_PROFILE_ZONE_TEXT(text, size) ZoneText(text, size)
#define IG_PROFILE_MARK_FRAME() FrameMark
#define IG_PROFILE_VALUE(name, value) TracyPlot(name, value)
#else
#define IG_PROFILE_ZONE() ((void)0)
#define IG_PROFILE_ZONE_NAMED(name) ((void)0)
#define IG_PROFILE_ZONE_TEXT(text, size) ((void)0)
#define IG_PROFILE_MARK_FRAME() ((void)0)
#define IG_PROFILE_VALUE(name, value) ((void)0)
#endif