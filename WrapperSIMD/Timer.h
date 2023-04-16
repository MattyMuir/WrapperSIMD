#pragma once
#include <chrono>

#define TIMING 1

#if TIMING
#define TIMER(name) Timer name
#define STOP_LOG(name) { name.Stop(false); std::cout << #name << " took: "; name.Log(); }
#define TIME_SCOPE(name) ScopedTimer name(#name)
#else
#define TIMER(name)
#define STOP_LOG(name)
#define TIME_SCOPE(name)
#endif

class Timer
{
	using Clock = std::chrono::steady_clock;
	using TimePoint = Clock::time_point;
	using Duration = Clock::duration;

public:
	Timer(bool start = true)
		: duration(0)
	{
		if (start) Start();
	}

	void Start()
	{
		start = Clock::now();
	}
	void Stop(bool log = true)
	{
		TimePoint end = Clock::now();
		duration += (end - start);

		if (log) Log();
	}

	void Log()
	{
		using namespace std::chrono;
		static constexpr uint64_t num = duration_cast<Duration>(1s).count();

		uint64_t count = duration.count();
		double millis = (double)count / num * 1000.0;

		if (millis < 1.0) std::cout << millis * 1000 << "us\n";
		else std::cout << millis << "ms\n";
	}
	Duration GetDuration() { return duration; }

protected:
	TimePoint start;
	Duration duration;
};

class ScopedTimer : Timer
{
public:
	ScopedTimer(const char* name_) : Timer(false), name(name_) { Start(); }
	~ScopedTimer() { Stop(false); std::cout << name << " took: "; Log(); }

protected:
	const char* name;
};