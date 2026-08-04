#pragma once

#include "Ignition/Core/Export.h"

#include <cstdint>

namespace Ignition
{
	struct Timestep
	{
		float Seconds = 0.0f;

		constexpr Timestep() = default;
		constexpr Timestep(float seconds) : Seconds(seconds) {}

		constexpr operator float() const { return Seconds; }

		constexpr float GetSeconds() const { return Seconds; }
		constexpr float GetMilliseconds() const { return Seconds * 1000.0f; }
	};

	class IGNITION_API Time
	{
	public:
		static void Initialize();
		static void Update();

		static Timestep GetDeltaTime();
		static Timestep GetUnscaledDeltaTime();

		static double GetElapsedTime();
		static double GetScaledElapsedTime();

		static uint64_t GetFrameCount();

		static float GetTimeScale();
		static void SetTimeScale(float timeScale);

		static float GetMaximumDeltaTime();
		static void SetMaximumDeltaTime(float maximumDeltaTime);

		static float GetFixedTimeStep();
		static void SetFixedTimeStep(float fixedTimeStep);

		static bool NextFixedStep();
		static float GetFixedAlpha();

		static float GetFramesPerSecond();
		static float GetAverageFrameTimeMilliseconds();
	};

	class IGNITION_API Timer
	{
	public:
		Timer();

		void Reset();

		float GetElapsedSeconds() const;
		float GetElapsedMilliseconds() const;

	private:
		double m_StartTime = 0.0;
	};
}