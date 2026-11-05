#include "web/serialization/job_json.h"

#include <sstream>

#include "web/serialization/json_utils.h"

namespace MiniFab::Web
{
std::string toJson(const std::vector<JobSnapshot>& jobs)
{
    std::ostringstream out;
    out << "[";

    for (std::size_t i = 0; i < jobs.size(); ++i)
    {
        if (i > 0)
            out << ",";

        const auto& snapshot = jobs[i];
        const auto& job = snapshot.job;

        out << "{\"jobId\":" << job.id
            << ",\"waferId\":";

        writeJsonString(out, job.waferId);

        out << ",\"recipeId\":" << job.recipeId
            << ",\"processModuleId\":" << job.processModuleId
            << ",\"status\":\""
            << toString(snapshot.status)
            << "\"}";
    }

    out << "]";
    return out.str();
}
}
