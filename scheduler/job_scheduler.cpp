#include <algorithm>
#include <vector>

#include "job_scheduler.h"

std::uint64_t JobScheduler::enqueue(
	const std::string& waferId,
	int recipeId,
	int processModuleId
)
{
	const bool alreadyQueued = std::ranges::any_of(
		queue_,
		[&waferId](const Job& job)
		{
			return job.waferId == waferId;
		}
	);

	const bool alreadyActive = activeJob_.has_value() && activeJob_->waferId == waferId;

	if (alreadyQueued || alreadyActive) return 0;

	const std::uint64_t jobId = nextJobId_++;
	Job job{
		.id = jobId,
		.waferId = waferId,
		.recipeId = recipeId,
		.processModuleId = processModuleId
	};
	jobs_.emplace(jobId, job);
	queue_.push_back(job);
	jobStatuses_[jobId] = JobStatus::Queued;
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

	jobStatuses_[jobId] = JobStatus::Cancelled;

	return true;
}

bool JobScheduler::dispatchNext(const std::function<bool(const Job&)>& tryStart) 
{
	if (activeJob_) return false;
	if (queue_.empty()) return false;

	if (!tryStart(queue_.front())) return false;

	activeJob_ = queue_.front();
	queue_.pop_front();

	jobStatuses_[activeJob_->id] = JobStatus::Running;

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

std::optional<JobStatus> JobScheduler::jobStatus(std::uint64_t jobId) const
{
	auto it = jobStatuses_.find(jobId);

	if (it == jobStatuses_.end()) return std::nullopt;

	return it->second;
}

bool JobScheduler::finishActiveJob(std::uint64_t jobId, OperationResult result)
{
	if (!activeJob_ || activeJob_->id != jobId) return false;

	JobStatus status = JobStatus::Failed;

	switch (result)
	{
		case OperationResult::Completed:
			status = JobStatus::Completed;
			break;

		case OperationResult::Cancelled:
			status = JobStatus::Cancelled;
			break;

		case OperationResult::Failed:
			status = JobStatus::Failed;
			break;

	}

	jobStatuses_[jobId] = status;
	activeJob_.reset();

	return true;
}

bool JobScheduler::rejectNextJob()
{
	if (queue_.empty()) return false;

	const auto jobId = queue_.front().id;

	jobStatuses_[jobId] = JobStatus::Rejected;

	queue_.pop_front();

	return true;
}

std::optional<Job> JobScheduler::nextQueuedJob() const
{
	if (queue_.empty()) return std::nullopt;

	return queue_.front();
}

std::vector<JobSnapshot> JobScheduler::jobsSnapshot() const
{
    std::vector<JobSnapshot> result;
    result.reserve(jobs_.size());

    for (const auto& [id, job] : jobs_)
    {
        auto status = jobStatuses_.find(id);

        if (status == jobStatuses_.end())
            continue;

        result.push_back(JobSnapshot{
            .job = job,
            .status = status->second
        });
    }

    std::sort(
        result.begin(),
        result.end(),
        [](const JobSnapshot& a, const JobSnapshot& b)
        {
            return a.job.id < b.job.id;
        }
    );

    return result;
}


