#include <optional>

#include <gtest/gtest.h>

#include "scheduler/job.h"
#include "scheduler/job_scheduler.h"


TEST(JobSchedulerTests, EnqueueAndDequeuePreserverFIFOOrder)
{
	JobScheduler scheduler;

	const auto id1 = scheduler.enqueue(2);
	const auto id2 = scheduler.enqueue(3);

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

TEST(JobSchedulerTests, CancelQueuedRemovesMiddleJob)
{
	JobScheduler scheduler;

	const auto id1 = scheduler.enqueue(10);
	const auto id2 = scheduler.enqueue(20);
	const auto id3 = scheduler.enqueue(30);

	bool cancelled = scheduler.cancelQueued(id2);

	ASSERT_TRUE(cancelled);

	cancelled = scheduler.cancelQueued(id2);

	EXPECT_FALSE(cancelled);

	ASSERT_EQ(scheduler.pendingCount(), 2);

	std::optional<Job> job1 = scheduler.dequeue();
	std::optional<Job> job3 = scheduler.dequeue();

	ASSERT_TRUE(job1.has_value());
	EXPECT_EQ(job1.value(), (Job{id1, 10}));

	ASSERT_TRUE(job3.has_value());
	EXPECT_EQ(job3.value(), (Job{id3, 30}));
}

