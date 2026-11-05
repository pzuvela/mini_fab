#pragma once

#include "scheduler/job.h"
#include "scheduler/job_status.h"


struct JobSnapshot
{
	Job job;
	Jobstatus status;
};
