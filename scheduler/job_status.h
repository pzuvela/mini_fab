#pragma once

enum class JobStatus
{
	Queued,
	Running,
	Completed,
	Failed,
	Cancelled,
	Rejected
};


namespace MiniFab::Web
{
    const char* toString(JobStatus status)
    {
        switch (status)
        {
            case JobStatus::Queued:    return "Queued";
            case JobStatus::Running:   return "Running";
            case JobStatus::Completed: return "Completed";
            case JobStatus::Failed:    return "Failed";
            case JobStatus::Cancelled: return "Cancelled";
            case JobStatus::Rejected:  return "Rejected";
        }

        return "Unknown";
    }
}
