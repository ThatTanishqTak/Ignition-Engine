#pragma once

#include "Ignition/Core/Export.h"

#include <format>
#include <string_view>

namespace Ignition
{
    class Log
    {
    public:
        enum class Level
        {
            Trace = 0,
            Info,
            Warn,
            Error,
            Critical
        };

        enum class Logger
        {
            Core = 0,
            Client
        };

        IGNITION_API static void Initialize();
        IGNITION_API static void Shutdown();

        IGNITION_API static void LogMessage(Logger logger, Level level, std::string_view message, const char* file, int line, const char* function);
    };
}

#define IG_INTERNAL_LOG(logger, level, ...) ::Ignition::Log::LogMessage(logger, level, std::format(__VA_ARGS__), __FILE__, __LINE__, __func__)

#define IG_CORE_TRACE(...) IG_INTERNAL_LOG(::Ignition::Log::Logger::Core, ::Ignition::Log::Level::Trace, __VA_ARGS__)
#define IG_CORE_INFO(...) IG_INTERNAL_LOG(::Ignition::Log::Logger::Core, ::Ignition::Log::Level::Info, __VA_ARGS__)
#define IG_CORE_WARN(...) IG_INTERNAL_LOG(::Ignition::Log::Logger::Core, ::Ignition::Log::Level::Warn, __VA_ARGS__)
#define IG_CORE_ERROR(...) IG_INTERNAL_LOG(::Ignition::Log::Logger::Core, ::Ignition::Log::Level::Error, __VA_ARGS__)
#define IG_CORE_CRITICAL(...) IG_INTERNAL_LOG(::Ignition::Log::Logger::Core, ::Ignition::Log::Level::Critical, __VA_ARGS__)

#define IG_APP_TRACE(...) IG_INTERNAL_LOG(::Ignition::Log::Logger::Client, ::Ignition::Log::Level::Trace, __VA_ARGS__)
#define IG_APP_INFO(...) IG_INTERNAL_LOG(::Ignition::Log::Logger::Client, ::Ignition::Log::Level::Info, __VA_ARGS__)
#define IG_APP_WARN(...) IG_INTERNAL_LOG(::Ignition::Log::Logger::Client, ::Ignition::Log::Level::Warn, __VA_ARGS__)
#define IG_APP_ERROR(...) IG_INTERNAL_LOG(::Ignition::Log::Logger::Client, ::Ignition::Log::Level::Error, __VA_ARGS__)
#define IG_APP_CRITICAL(...) IG_INTERNAL_LOG(::Ignition::Log::Logger::Client, ::Ignition::Log::Level::Critical, __VA_ARGS__)