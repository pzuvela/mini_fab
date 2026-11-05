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

TEST(JobSchedulerTests, DispatchAcceptedMovesJobToActive)
{
    JobScheduler scheduler;

    const auto jobId = scheduler.enqueue(10);

    bool callbackCalled = false;

    auto tryStart = [&callbackCalled](const Job& job) {
        callbackCalled = true;

        EXPECT_EQ(job.recipeId, 10);
        return true;
    };

    const bool dispatched = scheduler.dispatchNext(tryStart);

    EXPECT_TRUE(dispatched);
    EXPECT_TRUE(callbackCalled);
    EXPECT_EQ(scheduler.pendingCount(), 0);

    const std::optional<Job> activeJob = scheduler.activeJob();

    ASSERT_TRUE(activeJob.has_value());
    EXPECT_EQ(activeJob.value(), (Job{jobId, 10}));
}


TEST(JobSchedulerTests, DispatchRejectedKeepsJobQueued)
{
    JobScheduler scheduler;

    const auto jobId = scheduler.enqueue(20);

    bool callbackCalled = false;

    auto tryStart = [&callbackCalled](const Job& job) {
        callbackCalled = true;

        EXPECT_EQ(job.recipeId, 20);
        return false;
    };

    const bool dispatched = scheduler.dispatchNext(tryStart);

    EXPECT_FALSE(dispatched);
    EXPECT_TRUE(callbackCalled);

    // Rejected job must remain queued.
    EXPECT_EQ(scheduler.pendingCount(), 1);

    // No job should become active.
    EXPECT_FALSE(scheduler.activeJob().has_value());

    // Verify the original job was preserved.
    const std::optional<Job> queuedJob = scheduler.dequeue();

    ASSERT_TRUE(queuedJob.has_value());
    EXPECT_EQ(queuedJob.value(), (Job{jobId, 20}));
}

