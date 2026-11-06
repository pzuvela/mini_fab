#include <optional>

#include <gtest/gtest.h>

#include "scheduler/job.h"
#include "scheduler/job_scheduler.h"


TEST(JobSchedulerTests, EnqueueAndDequeuePreserverFIFOOrder)
{
	JobScheduler scheduler;

	const auto id1 = scheduler.enqueue("W1", 2, 1);
	const auto id2 = scheduler.enqueue("W2", 3, 2);

	ASSERT_EQ(scheduler.pendingCount(), 2);

	std::optional<Job> job1 = scheduler.dequeue();
	std::optional<Job> job2 = scheduler.dequeue();

	ASSERT_TRUE(job1.has_value());
	EXPECT_EQ(job1.value(), (Job{id1, "W1", 2, 1}));

	ASSERT_TRUE(job2.has_value());
	EXPECT_EQ(job2.value(), (Job{id2, "W2", 3, 2}));

	std::optional<Job> emptyJob = scheduler.dequeue();

	EXPECT_FALSE(emptyJob.has_value());
}

TEST(JobSchedulerTests, CancelQueuedRemovesMiddleJob)
{
	JobScheduler scheduler;

	const auto id1 = scheduler.enqueue("W1", 10, 1);
	const auto id2 = scheduler.enqueue("W2", 20, 2);
	const auto id3 = scheduler.enqueue("W3", 30, 3);

	bool cancelled = scheduler.cancelQueued(id2);

	ASSERT_TRUE(cancelled);

	cancelled = scheduler.cancelQueued(id2);

	EXPECT_FALSE(cancelled);

	ASSERT_EQ(scheduler.pendingCount(), 2);

	std::optional<Job> job1 = scheduler.dequeue();
	std::optional<Job> job3 = scheduler.dequeue();

	ASSERT_TRUE(job1.has_value());
	EXPECT_EQ(job1.value(), (Job{id1, "W1", 10, 1}));

	ASSERT_TRUE(job3.has_value());
	EXPECT_EQ(job3.value(), (Job{id3, "W3", 30, 3}));
}

TEST(JobSchedulerTests, DispatchAcceptedMovesJobToActive)
{
    JobScheduler scheduler;

    const auto jobId = scheduler.enqueue("W1",  10, 1);

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
    EXPECT_EQ(activeJob.value(), (Job{jobId, "W1", 10, 1}));
}


TEST(JobSchedulerTests, DispatchRejectedKeepsJobQueued)
{
    JobScheduler scheduler;

    const auto jobId = scheduler.enqueue("W1", 20, 1);

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
    EXPECT_EQ(queuedJob.value(), (Job{jobId, "W1",  20, 1}));
}

TEST(JobSchedulerTests, CompleteActiveJobAllowsNextDispatch)
{
    JobScheduler scheduler;

    const auto id1 = scheduler.enqueue("W1", 10, 1);
    const auto id2 = scheduler.enqueue("W2", 20, 2);

    auto accept = [](const Job&) { return true; };

    ASSERT_TRUE(scheduler.dispatchNext(accept));

    // Job 1 is active. Job 2 cannot start yet.
    EXPECT_FALSE(scheduler.dispatchNext(accept));
    EXPECT_EQ(scheduler.pendingCount(), 1);

    // Complete Job 1.
    EXPECT_TRUE(scheduler.completeActiveJob(id1));
    EXPECT_FALSE(scheduler.activeJob().has_value());

    // Job 2 can now start.
    EXPECT_TRUE(scheduler.dispatchNext(accept));

    ASSERT_TRUE(scheduler.activeJob().has_value());
    EXPECT_EQ(scheduler.activeJob()->id, id2);
    EXPECT_EQ(scheduler.pendingCount(), 0);
}

TEST(JobSchedulerTests, CompleteActiveJobRejectsWhenNoJobIsActive)
{
    JobScheduler scheduler;
    const auto jobId = scheduler.enqueue("W1", 10, 1);

    EXPECT_FALSE(scheduler.completeActiveJob(jobId));
    EXPECT_EQ(scheduler.pendingCount(), 1);
}

TEST(JobSchedulerTests, CompleteActiveJobRejectsWrongId)
{
    JobScheduler scheduler;
    const auto activeId = scheduler.enqueue("W1", 10, 1);
    const auto otherId = scheduler.enqueue("W2", 20, 2);

    ASSERT_TRUE(scheduler.dispatchNext([](const Job&) { return true; }));
    EXPECT_FALSE(scheduler.completeActiveJob(otherId));
    ASSERT_TRUE(scheduler.activeJob().has_value());
    EXPECT_EQ(scheduler.activeJob()->id, activeId);
    EXPECT_EQ(scheduler.pendingCount(), 1);
}

TEST(JobSchedulerTests, CompleteActiveJobRejectsDuplicateCompletion)
{
    JobScheduler scheduler;
    const auto jobId = scheduler.enqueue("W1", 10, 1);

    ASSERT_TRUE(scheduler.dispatchNext([](const Job&) { return true; }));
    ASSERT_TRUE(scheduler.completeActiveJob(jobId));
    EXPECT_FALSE(scheduler.completeActiveJob(jobId));
    EXPECT_FALSE(scheduler.activeJob().has_value());
}

TEST(JobSchedulerTest, TracksJobLifecycle)
{
    JobScheduler scheduler;

    const auto id = scheduler.enqueue("W1", 1, 1);

    ASSERT_EQ(scheduler.jobStatus(id), JobStatus::Queued);

    ASSERT_TRUE(scheduler.dispatchNext(
        [](const Job&) { return true; }
    ));

    EXPECT_EQ(scheduler.jobStatus(id), JobStatus::Running);

    ASSERT_TRUE(scheduler.finishActiveJob(
        id, OperationResult::Completed
    ));

    EXPECT_EQ(scheduler.jobStatus(id), JobStatus::Completed);
}

TEST(JobSchedulerTest, TracksFailedJob)
{
    JobScheduler scheduler;

    const auto id = scheduler.enqueue("W1", 1, 1);

    ASSERT_TRUE(scheduler.dispatchNext(
        [](const Job&) { return true; }
    ));

    ASSERT_TRUE(scheduler.finishActiveJob(
        id, OperationResult::Failed
    ));

    EXPECT_EQ(scheduler.jobStatus(id), JobStatus::Failed);
}

TEST(JobSchedulerTest, TracksCancelledQueuedJob)
{
    JobScheduler scheduler;

    const auto id = scheduler.enqueue("W1", 1, 1);

    ASSERT_TRUE(scheduler.cancelQueued(id));

    EXPECT_EQ(scheduler.jobStatus(id), JobStatus::Cancelled);
    EXPECT_EQ(scheduler.pendingCount(), 0);
}

TEST(JobSchedulerTest, UnknownJobHasNoStatus)
{
    JobScheduler scheduler;

    EXPECT_EQ(scheduler.jobStatus(999), std::nullopt);
}

