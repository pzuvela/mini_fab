#pragma once

#include <cstdint>
#include <deque>
#include <optional>

#include "job.h"

class JobScheduler
{
private:
	std::deque<Job> queue_;
	std::uint64_t nextJobId_ = 1;

public:
	std::uint64_t enqueue(int recipeId);
	std::optional<Job> dequeue();
	bool cancelQueued(std::uint64_t jobId);
	std::size_t pendingCount() const;
};
