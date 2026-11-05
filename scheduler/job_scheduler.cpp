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

bool JobScheduler::dispatchNext(const std::function<bool(const Job&)>& tryStart) 
{
	if (activeJob_) return false;
	if (queue_.empty()) return false;

	std::optional<Job> job = queue_.front();

	if (!tryStart(queue_.front())) return false;

	activeJob_ = queue_.front();
	queue_.pop_front();

	return true;
}

std::optional<Job> JobScheduler::activeJob() const
{
	return activeJob_;
}

bool JobScheduler::completeActiveJob(std::uint64_t jobId)
{
	if (!activeJob_) return false;

	if (activeJob_->id != jobId) return false;

	activeJob_.reset();

	return true;
}
