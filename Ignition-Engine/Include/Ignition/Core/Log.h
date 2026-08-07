#pragma once

#include "Ignition/Core/Export.h"

#include <memory>

#include <spdlog/spdlog.h>

namespace Ignition
{
    class Log
    {
    public:
        IGNITION_API static void Initialize();
        IGNITION_API static void Shutdown();

        IGNITION_API static spdlog::logger* GetCoreLogger();
        IGNITION_API static spdlog::logger* GetClientLogger();

    private:
        static std::shared_ptr<spdlog::logger> s_CoreLogger;
        static std::shared_ptr<spdlog::logger> s_ClientLogger;
    };
}

#define IG_INTERNAL_LOG(logger, level, ...) (logger)->log(spdlog::source_loc{ __FILE__, __LINE__, SPDLOG_FUNCTION }, level, __VA_ARGS__)

#define IG_CORE_TRACE(...) IG_INTERNAL_LOG(::Ignition::Log::GetCoreLogger(), spdlog::level::trace, __VA_ARGS__)
#define IG_CORE_INFO(...) IG_INTERNAL_LOG(::Ignition::Log::GetCoreLogger(), spdlog::level::info, __VA_ARGS__)
#define IG_CORE_WARN(...) IG_INTERNAL_LOG(::Ignition::Log::GetCoreLogger(), spdlog::level::warn, __VA_ARGS__)
#define IG_CORE_ERROR(...) IG_INTERNAL_LOG(::Ignition::Log::GetCoreLogger(), spdlog::level::err, __VA_ARGS__)
#define IG_CORE_CRITICAL(...) IG_INTERNAL_LOG(::Ignition::Log::GetCoreLogger(), spdlog::level::critical, __VA_ARGS__)

#define IG_APP_TRACE(...) IG_INTERNAL_LOG(::Ignition::Log::GetClientLogger(), spdlog::level::trace, __VA_ARGS__)
#define IG_APP_INFO(...) IG_INTERNAL_LOG(::Ignition::Log::GetClientLogger(), spdlog::level::info, __VA_ARGS__)
#define IG_APP_WARN(...) IG_INTERNAL_LOG(::Ignition::Log::GetClientLogger(), spdlog::level::warn, __VA_ARGS__)
#define IG_APP_ERROR(...) IG_INTERNAL_LOG(::Ignition::Log::GetClientLogger(), spdlog::level::err, __VA_ARGS__)
#define IG_APP_CRITICAL(...) IG_INTERNAL_LOG(::Ignition::Log::GetClientLogger(), spdlog::level::critical, __VA_ARGS__)