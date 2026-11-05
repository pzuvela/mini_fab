#pragma once

#include <cstdint>
#include <deque>
#include <optional>
#include <functional>
#include <unordered_map>
#include <vector>

#include "job_status.h"
#include "job.h"
#include "equipment/operation_result.h"
#include "job_snapshot.h"


class JobScheduler
{
private:
	std::deque<Job> queue_;
	std::uint64_t nextJobId_ = 1;
	std::optional<Job> activeJob_;
	std::unordered_map<std::uint64_t, JobStatus> jobStatuses_;
	std::unordered_map<std::uint64_t, Job> jobs_;

public:
	std::uint64_t enqueue(const std::string& waferId, int recipeId, int processModuleId);
	std::optional<Job> dequeue();
	bool cancelQueued(std::uint64_t jobId);
	std::size_t pendingCount() const;
	bool dispatchNext(const std::function<bool(const Job&)>& tryStart);
	std::optional<Job> activeJob() const;
	bool completeActiveJob(std::uint64_t jobId);
	std::optional<JobStatus> jobStatus(std::uint64_t jobId) const;
	bool finishActiveJob(std::uint64_t jobId, OperationResult result);
	bool rejectNextJob();
	std::optional<Job> nextQueuedJob() const;
	std::vector<JobSnapshot> jobsSnapshot() const;
};
