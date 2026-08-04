#pragma once

#include "Ignition/Core/Export.h"

#include <memory>
#include <filesystem>
#include <functional>
#include <string>

#include <spdlog/spdlog.h>

namespace Ignition
{
    class IGNITION_API Log
    {
    public:
        static void Initialize();
        static void Shutdown();

        static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
        static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }

    private:
        static std::shared_ptr<spdlog::logger> s_CoreLogger;
        static std::shared_ptr<spdlog::logger> s_ClientLogger;
    };
}

#define INTERNAL_LOG(logger, level, ...) (logger)->log(spdlog::source_loc{ __FILE__, __LINE__, SPDLOG_FUNCTION }, level, __VA_ARGS__)

#define CORE_TRACE(...) INTERNAL_LOG(::Ignition::Log::GetCoreLogger(), spdlog::level::trace, __VA_ARGS__)
#define CORE_INFO(...) INTERNAL_LOG(::Ignition::Log::GetCoreLogger(), spdlog::level::info, __VA_ARGS__)
#define CORE_WARN(...) INTERNAL_LOG(::Ignition::Log::GetCoreLogger(), spdlog::level::warn, __VA_ARGS__)
#define CORE_ERROR(...) INTERNAL_LOG(::Ignition::Log::GetCoreLogger(), spdlog::level::err, __VA_ARGS__)
#define CORE_CRITICAL(...) INTERNAL_LOG(::Ignition::Log::GetCoreLogger(), spdlog::level::critical, __VA_ARGS__)

#define APP_TRACE(...) INTERNAL_LOG(::Ignition::Log::GetClientLogger(), spdlog::level::trace, __VA_ARGS__)
#define APP_INFO(...) INTERNAL_LOG(::Ignition::Log::GetClientLogger(), spdlog::level::info, __VA_ARGS__)
#define APP_WARN(...) INTERNAL_LOG(::Ignition::Log::GetClientLogger(), spdlog::level::warn, __VA_ARGS__)
#define APP_ERROR(...) INTERNAL_LOG(::Ignition::Log::GetClientLogger(), spdlog::level::err, __VA_ARGS__)
#define APP_CRITICAL(...) INTERNAL_LOG(::Ignition::Log::GetClientLogger(), spdlog::level::critical, __VA_ARGS__)