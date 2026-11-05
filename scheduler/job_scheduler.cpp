#include <algorithm>

#include "job_scheduler.h"

std::uint64_t JobScheduler::enqueue(int recipeId)
{
	const std::uint64_t jobId = nextJobId_++;
	Job job{jobId, recipeId};
	queue_.push_back(job);
	return jobId;
}

std::size_t JobScheduler::pendingCount() const  // const after the method promises not to modify the observable state of the JobScheduler obj
{
	return queue_.size();
}

std::optional<Job> JobScheduler::dequeue()
{
	if (queue_.empty()) return std::nullopt;

	Job job = queue_.front();

	queue_.pop_front();

	return job;
}

bool JobScheduler::cancelQueued(std::uint64_t jobId) 
{
	auto it = std::find_if(
		queue_.begin(),
		queue_.end(),
		[&jobId](const Job& job) {
			return job.id == jobId;
		}
	);

	if (it == queue_.end()) return false;

	queue_.erase(it);

	return true;
}
