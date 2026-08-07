#include "Ignition/Core/Time.h"

#include "Ignition/Core/Log.h"

#include <chrono>
#include <cmath>

namespace Ignition
{
	namespace
	{
		using Clock = std::chrono::steady_clock;

		constexpr int MaximumFixedStepsPerFrame = 8;

		Clock::time_point g_StartTime{};
		Clock::time_point g_LastFrameTime{};

		double g_ElapsedTime = 0.0;
		double g_ScaledElapsedTime = 0.0;

		float g_DeltaTime = 0.0f;
		float g_UnscaledDeltaTime = 0.0f;

		float g_TimeScale = 1.0f;
		float g_MaximumDeltaTime = 0.25f;

		float g_FixedTimeStep = 1.0f / 60.0f;
		float g_FixedAccumulator = 0.0f;
		int g_FixedStepsThisFrame = 0;

		uint64_t g_FrameCount = 0;

		float g_SampleAccumulator = 0.0f;
		uint32_t g_SampleFrames = 0;
		float g_FramesPerSecond = 0.0f;
		float g_AverageFrameTime = 0.0f;

		double CurrentSeconds()
		{
			return std::chrono::duration<double>(Clock::now() - g_StartTime).count();
		}
	}

	void Time::Initialize()
	{
		g_StartTime = Clock::now();
		g_LastFrameTime = g_StartTime;

		g_ElapsedTime = 0.0;
		g_ScaledElapsedTime = 0.0;

		g_DeltaTime = 0.0f;
		g_UnscaledDeltaTime = 0.0f;

		g_FixedAccumulator = 0.0f;
		g_FixedStepsThisFrame = 0;

		g_FrameCount = 0;

		g_SampleAccumulator = 0.0f;
		g_SampleFrames = 0;
		g_FramesPerSecond = 0.0f;
		g_AverageFrameTime = 0.0f;
	}

	void Time::Update()
	{
		const Clock::time_point currentTime = Clock::now();

		g_UnscaledDeltaTime = std::chrono::duration<float>(currentTime - g_LastFrameTime).count();
		g_LastFrameTime = currentTime;

		if (g_UnscaledDeltaTime > g_MaximumDeltaTime)
		{
			g_UnscaledDeltaTime = g_MaximumDeltaTime;
		}

		g_DeltaTime = g_UnscaledDeltaTime * g_TimeScale;

		g_ElapsedTime += static_cast<double>(g_UnscaledDeltaTime);
		g_ScaledElapsedTime += static_cast<double>(g_DeltaTime);

		++g_FrameCount;

		g_FixedAccumulator += g_DeltaTime;
		g_FixedStepsThisFrame = 0;

		g_SampleAccumulator += g_UnscaledDeltaTime;
		++g_SampleFrames;

		if (g_SampleAccumulator >= 1.0f)
		{
			g_FramesPerSecond = static_cast<float>(g_SampleFrames) / g_SampleAccumulator;
			g_AverageFrameTime = (g_SampleAccumulator / static_cast<float>(g_SampleFrames)) * 1000.0f;

			g_SampleAccumulator = 0.0f;
			g_SampleFrames = 0;
		}
	}

	Timestep Time::GetDeltaTime()
	{
		return Timestep(g_DeltaTime);
	}

	Timestep Time::GetUnscaledDeltaTime()
	{
		return Timestep(g_UnscaledDeltaTime);
	}

	double Time::GetElapsedTime()
	{
		return g_ElapsedTime;
	}

	double Time::GetScaledElapsedTime()
	{
		return g_ScaledElapsedTime;
	}

	uint64_t Time::GetFrameCount()
	{
		return g_FrameCount;
	}

	float Time::GetTimeScale()
	{
		return g_TimeScale;
	}

	void Time::SetTimeScale(float timeScale)
	{
		if (timeScale < 0.0f)
		{
			IG_CORE_WARN("Negative time scale requested ({}), clamping to zero", timeScale);
			timeScale = 0.0f;
		}

		g_TimeScale = timeScale;
	}

	float Time::GetMaximumDeltaTime()
	{
		return g_MaximumDeltaTime;
	}

	void Time::SetMaximumDeltaTime(float maximumDeltaTime)
	{
		if (maximumDeltaTime <= 0.0f)
		{
			IG_CORE_WARN("Invalid maximum delta time ({}), ignoring", maximumDeltaTime);

			return;
		}

		g_MaximumDeltaTime = maximumDeltaTime;
	}

	float Time::GetFixedTimeStep()
	{
		return g_FixedTimeStep;
	}

	void Time::SetFixedTimeStep(float fixedTimeStep)
	{
		if (fixedTimeStep <= 0.0f)
		{
			IG_CORE_WARN("Invalid fixed time step ({}), ignoring", fixedTimeStep);

			return;
		}

		g_FixedTimeStep = fixedTimeStep;
	}

	bool Time::NextFixedStep()
	{
		if (g_FixedAccumulator < g_FixedTimeStep)
		{
			return false;
		}

		if (g_FixedStepsThisFrame >= MaximumFixedStepsPerFrame)
		{
			const float remainder = std::fmod(g_FixedAccumulator, g_FixedTimeStep);
			IG_CORE_WARN("Fixed step budget exhausted, discarding {} seconds of accumulated time", g_FixedAccumulator - remainder);
			g_FixedAccumulator = remainder;

			return false;
		}

		g_FixedAccumulator -= g_FixedTimeStep;
		++g_FixedStepsThisFrame;

		return true;
	}

	float Time::GetFixedAlpha()
	{
		if (g_FixedTimeStep <= 0.0f)
		{
			return 0.0f;
		}

		return g_FixedAccumulator / g_FixedTimeStep;
	}

	float Time::GetFramesPerSecond()
	{
		return g_FramesPerSecond;
	}

	float Time::GetAverageFrameTimeMilliseconds()
	{
		return g_AverageFrameTime;
	}

	Timer::Timer()
	{
		Reset();
	}

	void Timer::Reset()
	{
		m_StartTime = CurrentSeconds();
	}

	float Timer::GetElapsedSeconds() const
	{
		return static_cast<float>(CurrentSeconds() - m_StartTime);
	}

	float Timer::GetElapsedMilliseconds() const
	{
		return GetElapsedSeconds() * 1000.0f;
	}
}