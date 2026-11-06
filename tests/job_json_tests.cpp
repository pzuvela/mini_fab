
#include <gtest/gtest.h>

#include <vector>

#include "web/serialization/job_json.h"

TEST(JobJsonTest, SerializesEmptyCollection)
{
    const std::vector<JobSnapshot> jobs;

    EXPECT_EQ(MiniFab::Web::toJson(jobs), "[]");
}

TEST(JobJsonTest, SerializesQueuedJob)
{
    const JobSnapshot snapshot{
        Job{1, "W1", 10, 1},
        JobStatus::Queued
    };

    EXPECT_EQ(
        MiniFab::Web::toJson({snapshot}),
        R"([{"jobId":1,"waferId":"W1","recipeId":10,"processModuleId":1,"status":"Queued"}])"
    );
}

TEST(JobJsonTest, SerializesMultipleStatuses)
{
    const std::vector<JobSnapshot> jobs{
        {Job{1, "W1", 10, 1}, JobStatus::Running},
        {Job{2, "W2", 20, 1}, JobStatus::Completed}
    };

    EXPECT_EQ(
        MiniFab::Web::toJson(jobs),
        R"([{"jobId":1,"waferId":"W1","recipeId":10,"processModuleId":1,"status":"Running"},{"jobId":2,"waferId":"W2","recipeId":20,"processModuleId":1,"status":"Completed"}])"
    );
}

TEST(JobJsonTest, EscapesWaferId)
{
    const JobSnapshot snapshot{
        Job{3, "W\"3\\test", 10, 1},
        JobStatus::Failed
    };

    EXPECT_EQ(
        MiniFab::Web::toJson({snapshot}),
        R"([{"jobId":3,"waferId":"W\"3\\test","recipeId":10,"processModuleId":1,"status":"Failed"}])"
    );
}
