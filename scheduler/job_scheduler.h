#pragma once

#include <cstdint>
#include <deque>
#include <optional>
#include <functional>
#include "job.h"


class JobScheduler
{
private:
	std::deque<Job> queue_;
	std::uint64_t nextJobId_ = 1;
	std::optional<Job> activeJob_;

public:
	std::uint64_t enqueue(int recipeId);
	std::optional<Job> dequeue();
	bool cancelQueued(std::uint64_t jobId);
	std::size_t pendingCount() const;
	bool dispatchNext(const std::function<bool(const Job&)>& tryStart);
	std::optional<Job> activeJob() const;
	bool completeActiveJob(std::uint64_t jobId);
};
