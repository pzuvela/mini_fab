#pragma once

#include <string>
#include <vector>

#include "scheduler/job_snapshot.h"

namespace MiniFab::Web
{
    std::string toJson(const std::vector<JobSnapshot>& jobs);
}

