#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include <functional>

#include "controller/mini_fab_controller.h"
#include "persistence/sqlite_recipe_repository.h"

using namespace std::chrono_literals;

namespace
{
    bool waitUntil(
        const std::function<bool()>& condition,
        std::chrono::milliseconds timeout = 5s)
    {
        const auto deadline =
            std::chrono::steady_clock::now() + timeout;

        while (std::chrono::steady_clock::now() < deadline)
        {
            if (condition())
                return true;

            std::this_thread::sleep_for(10ms);
        }

        return condition();
    }
}


TEST(MiniFabControllerTest, StartsWithWaferAtLoadPort)
{
    MiniFabController controller;

    EquipmentSnapshot snapshot = controller.status();

    EXPECT_EQ(snapshot.state, EquipmentState::Idle);

    ASSERT_TRUE(snapshot.waferLocation.has_value());

    EXPECT_EQ(
        snapshot.waferLocation.value(),
        (Location{Station::LoadPort, 3})
    );
}

TEST(MiniFabControllerTest, WaitForCompletionReturnsFalseWhenNoJobActive)
{
    MiniFabController controller;

    EXPECT_FALSE(controller.waitForActiveJobCompletion());
}

TEST(MiniFabControllerTest, CompletedJobAllowsNextJobToDispatch)
{
    SqliteRecipeRepository repository(":memory:");
    RecipeManager manager(repository);
    MiniFabController controller(manager);

    ASSERT_TRUE(controller.addRecipe({1, "Fast Etch", 0.01}));

    const auto job1 = controller.enqueueJob("W1", 1, 1);
    const auto job2 = controller.enqueueJob("W1", 1, 1);

    EXPECT_NE(job1, job2);
    EXPECT_EQ(controller.pendingJobCount(), 2);

    // First job starts.
    ASSERT_TRUE(controller.dispatchNextJob());
    EXPECT_EQ(controller.pendingJobCount(), 1);

    // Scheduler must not dispatch another active job.
    EXPECT_FALSE(controller.dispatchNextJob());

    // First job finishes and releases scheduler.
    ASSERT_TRUE(controller.waitForActiveJobCompletion());
    EXPECT_EQ(controller.status().state, EquipmentState::Idle);

    // Second job can now run.
    ASSERT_TRUE(controller.dispatchNextJob());
    EXPECT_EQ(controller.pendingJobCount(), 0);

    ASSERT_TRUE(controller.waitForActiveJobCompletion());
    EXPECT_EQ(controller.status().state, EquipmentState::Idle);

    // No more jobs.
    EXPECT_FALSE(controller.dispatchNextJob());
    EXPECT_FALSE(controller.waitForActiveJobCompletion());
}

TEST(MiniFabControllerTest, RejectedJobRemainsQueued)
{
    SqliteRecipeRepository repository(":memory:");
    RecipeManager manager(repository);
    MiniFabController controller(manager);

    ASSERT_TRUE(controller.addRecipe({1, "Fast Etch", 0.01}));

    // PM2 is not supported yet.
    controller.enqueueJob("W1", 1, 2);

    ASSERT_EQ(controller.pendingJobCount(), 1);

    EXPECT_FALSE(controller.dispatchNextJob());

    // Rejected dispatch must not remove the job.
    EXPECT_EQ(controller.pendingJobCount(), 1);
    EXPECT_EQ(controller.status().state, EquipmentState::Idle);

    // Retrying still leaves the job queued.
    EXPECT_FALSE(controller.dispatchNextJob());
    EXPECT_EQ(controller.pendingJobCount(), 1);

    EXPECT_FALSE(controller.waitForActiveJobCompletion());
}

TEST(MiniFabControllerTest, AutoSchedulerProcessesQueuedJobs)
{
    SqliteRecipeRepository repository(":memory:");
    RecipeManager manager(repository);
    MiniFabController controller(manager);

    ASSERT_TRUE(controller.addRecipe({1, "Fast Etch", 0.01}));

    controller.startScheduler();

    controller.enqueueJob("W1", 1, 1);
    controller.enqueueJob("W1", 1, 1);

    // Both jobs should be dispatched automatically.
    ASSERT_TRUE(waitUntil([&]()
    {
        return controller.pendingJobCount() == 0;
    }));

    // Queue empty does not necessarily mean processing finished.
    // Stop joins the worker, allowing the active job to finish.
    controller.stopScheduler();

    EXPECT_EQ(controller.status().state, EquipmentState::Idle);
    EXPECT_FALSE(controller.waitForActiveJobCompletion());
}

TEST(MiniFabControllerTest, AutoSchedulerWakesForNewJob)
{
    SqliteRecipeRepository repository(":memory:");
    RecipeManager manager(repository);
    MiniFabController controller(manager);

    ASSERT_TRUE(controller.addRecipe({1, "Fast Etch", 0.01}));

    // Start with an empty queue.
    controller.startScheduler();

    EXPECT_EQ(controller.pendingJobCount(), 0);

    // Worker should wake after enqueueJob() notifies it.
    controller.enqueueJob("W1", 1, 1);

    ASSERT_TRUE(waitUntil([&]()
    {
        return controller.pendingJobCount() == 0;
    }));

    controller.stopScheduler();

    EXPECT_EQ(controller.status().state, EquipmentState::Idle);
}

TEST(MiniFabControllerTest, SchedulerCanBeStoppedTwice)
{
    MiniFabController controller;

    controller.startScheduler();

    EXPECT_NO_THROW(controller.stopScheduler());
    EXPECT_NO_THROW(controller.stopScheduler());
}

TEST(MiniFabControllerTest, DestructorStopsScheduler)
{
    EXPECT_NO_THROW(
    {
        MiniFabController controller;
        controller.startScheduler();

        // No explicit stopScheduler().
        // Destructor must request stop and join.
    });
}


