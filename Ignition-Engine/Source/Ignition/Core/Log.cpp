#include "Ignition/Core/Log.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <memory>

namespace Ignition
{
    namespace
    {
        std::shared_ptr<spdlog::logger> s_CoreLogger;
        std::shared_ptr<spdlog::logger> s_ClientLogger;

        spdlog::level::level_enum ToSpdlogLevel(Log::Level level)
        {
            switch (level)
            {
                case Log::Level::Trace:
                    return spdlog::level::trace;
                case Log::Level::Info:
                    return spdlog::level::info;
                case Log::Level::Warn:
                    return spdlog::level::warn;
                case Log::Level::Error:
                    return spdlog::level::err;
                case Log::Level::Critical:
                    return spdlog::level::critical;
            }

            return spdlog::level::info;
        }
    }

    void Log::Initialize()
    {
        auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        consoleSink->set_pattern("%^[%T] [%n] %v%$");

        s_CoreLogger = std::make_shared<spdlog::logger>("IGNITION", consoleSink);
        s_CoreLogger->set_level(spdlog::level::trace);
        s_CoreLogger->flush_on(spdlog::level::trace);
        spdlog::register_logger(s_CoreLogger);

        s_ClientLogger = std::make_shared<spdlog::logger>("SANDBOX", consoleSink);
        s_ClientLogger->set_level(spdlog::level::trace);
        s_ClientLogger->flush_on(spdlog::level::trace);
        spdlog::register_logger(s_ClientLogger);
    }

    void Log::Shutdown()
    {
        s_ClientLogger.reset();
        s_CoreLogger.reset();
        spdlog::drop_all();
    }

    void Log::LogMessage(Logger logger, Level level, std::string_view message, const char* file, int line, const char* function)
    {
        spdlog::logger* target = logger == Logger::Core ? s_CoreLogger.get() : s_ClientLogger.get();

        if (!target)
        {
            return;
        }

        target->log(spdlog::source_loc{ file, line, function }, ToSpdlogLevel(level), message);
    }
}