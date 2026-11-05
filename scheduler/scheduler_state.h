#pragma once

enum class SchedulerState
{
	Stopped,
	Running,
	Faulted
};

namespace MiniFab::Web
{
	const char* toString(SchedulerState state)
	{
		switch (state)
		{
			case SchedulerState::Stopped: return "Stopped";
			case SchedulerState::Running: return "Running";
			case SchedulerState::Faulted: return "Faulted";
		}

		return "Unknown";
	}
}
