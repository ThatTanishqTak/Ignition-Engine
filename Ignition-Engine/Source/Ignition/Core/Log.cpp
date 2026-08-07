#include <Ignition/Core/Log.h>

#include <spdlog/sinks/stdout_color_sinks.h>

namespace Ignition
{
    std::shared_ptr<spdlog::logger> Log::s_CoreLogger;
    std::shared_ptr<spdlog::logger> Log::s_ClientLogger;

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

    spdlog::logger* Log::GetCoreLogger()
    {
        return s_CoreLogger.get();
    }

    spdlog::logger* Log::GetClientLogger()
    {
        return s_ClientLogger.get();
    }
}