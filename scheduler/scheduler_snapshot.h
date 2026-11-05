#pragma once

#include <optional>
#include <string>
#include <cstddef>
#include <cstdint>

#include "scheduler_state.h"

struct SchedulerSnapshot
{
	SchedulerState state;
	std::size_t pendingJobs;
	std::optional<std::uint64_t> activeJobId;
	std::string error;
};
