#include <optional>

#include <gtest/gtest.h>

#include "scheduler/job.h"
#include "scheduler/job_scheduler.h"


TEST(JobSchedulerTests, EnqueueAndDequeuePreserverFIFOOrder)
{
	JobScheduler scheduler;

	auto id1 = scheduler.enqueue(2);
	auto id2 = scheduler.enqueue(3);

	ASSERT_EQ(scheduler.pendingCount(), 2);

	std::optional<Job> job1 = scheduler.dequeue();
	std::optional<Job> job2 = scheduler.dequeue();

	ASSERT_TRUE(job1.has_value());
	EXPECT_EQ(job1.value(), (Job{id1, 2}));

	ASSERT_TRUE(job2.has_value());
	EXPECT_EQ(job2.value(), (Job{id2, 3}));

	std::optional<Job> emptyJob = scheduler.dequeue();

	EXPECT_FALSE(emptyJob.has_value());
}

