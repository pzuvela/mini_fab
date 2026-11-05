#include <gtest/gtest.h>

#include "controller/mini_fab_controller.h"
#include "persistence/sqlite_recipe_repository.h"


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
