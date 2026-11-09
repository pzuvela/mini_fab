#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include <functional>
#include <algorithm>

#include "controller/mini_fab_controller.h"
#include "persistence/sqlite_recipe_repository.h"
#include "scheduler/dispatch_result.h"
#include "mocks/fault_injecting_controller.h"

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
    ASSERT_EQ(controller.dispatchNextJob(), DispatchResult::Started);
    EXPECT_EQ(controller.pendingJobCount(), 1);

    // Scheduler must not dispatch another active job.
    EXPECT_EQ(controller.dispatchNextJob(), DispatchResult::Busy);

    // First job finishes and releases scheduler.
    ASSERT_TRUE(controller.waitForActiveJobCompletion());
    EXPECT_EQ(controller.status().state, EquipmentState::Idle);

    // Second job can now run.
    ASSERT_EQ(controller.dispatchNextJob(), DispatchResult::Started);
    EXPECT_EQ(controller.pendingJobCount(), 0);

    ASSERT_TRUE(controller.waitForActiveJobCompletion());
    EXPECT_EQ(controller.status().state, EquipmentState::Idle);

    // No more jobs.
    EXPECT_EQ(controller.dispatchNextJob(), DispatchResult::Empty);
    EXPECT_FALSE(controller.waitForActiveJobCompletion());
}

TEST(MiniFabControllerTest, InvalidJobIsRejectedAndRemovedFromQueue)
{
    SqliteRecipeRepository repository(":memory:");
    RecipeManager manager(repository);
    MiniFabController controller(manager);

    ASSERT_TRUE(controller.addRecipe({1, "Fast Etch", 0.01}));

    // PM2 is not supported yet.
    const auto jobId = controller.enqueueJob("W1", 1, 2);

    EXPECT_EQ(controller.jobStatus(jobId), JobStatus::Queued);
    EXPECT_EQ(controller.pendingJobCount(), 1);

    // Invalid job cannot be dispatched.
    EXPECT_EQ(controller.dispatchNextJob(), DispatchResult::Rejected);

    // Job is rejected and removed from the queue.
    EXPECT_EQ(controller.jobStatus(jobId), JobStatus::Rejected);
    EXPECT_EQ(controller.pendingJobCount(), 0);

    // No job should be running.
    EXPECT_EQ(controller.status().state, EquipmentState::Idle);
    EXPECT_FALSE(controller.waitForActiveJobCompletion());

    // No jobs remain to dispatch.
    EXPECT_EQ(controller.dispatchNextJob(), DispatchResult::Empty);

    // Rejection status persists.
    EXPECT_EQ(controller.jobStatus(jobId), JobStatus::Rejected);
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

TEST(MiniFabControllerTest, AutoSchedulerProcessesQueuedJobs)
{
    SqliteRecipeRepository repository(":memory:");
    RecipeManager manager(repository);
    MiniFabController controller(manager);

    ASSERT_TRUE(controller.addRecipe({1, "Fast Etch", 0.01}));

    const auto job1 = controller.enqueueJob("W1", 1, 1);
    const auto job2 = controller.enqueueJob("W1", 1, 1);

    EXPECT_EQ(controller.jobStatus(job1), JobStatus::Queued);
    EXPECT_EQ(controller.jobStatus(job2), JobStatus::Queued);

    controller.startScheduler();

    // Wait for the first job to complete.
    ASSERT_TRUE(waitUntil([&]()
    {
        return controller.jobStatus(job1) == JobStatus::Completed;
    }));

    // The second job must not have completed before the first.
    // It may be queued or running by now.
    const auto secondStatus = controller.jobStatus(job2);

    ASSERT_TRUE(secondStatus.has_value());
    EXPECT_NE(*secondStatus, JobStatus::Failed);
    EXPECT_NE(*secondStatus, JobStatus::Cancelled);

    // Wait for the second job to complete.
    ASSERT_TRUE(waitUntil([&]()
    {
        return controller.jobStatus(job2) == JobStatus::Completed;
    }));

    controller.stopScheduler();

    EXPECT_EQ(controller.pendingJobCount(), 0);
    EXPECT_EQ(controller.status().state, EquipmentState::Idle);

    EXPECT_EQ(controller.jobStatus(job1), JobStatus::Completed);
    EXPECT_EQ(controller.jobStatus(job2), JobStatus::Completed);
}

TEST(MiniFabControllerTest, AutoSchedulerWakesForNewJob)
{
    SqliteRecipeRepository repository(":memory:");
    RecipeManager manager(repository);
    MiniFabController controller(manager);

    ASSERT_TRUE(controller.addRecipe({1, "Fast Etch", 0.01}));

    controller.startScheduler();

    EXPECT_EQ(controller.pendingJobCount(), 0);

    const auto jobId = controller.enqueueJob("W1", 1, 1);

    ASSERT_TRUE(waitUntil([&]()
    {
        return controller.jobStatus(jobId) == JobStatus::Completed;
    }));

    controller.stopScheduler();

    EXPECT_EQ(controller.jobStatus(jobId), JobStatus::Completed);
    EXPECT_EQ(controller.status().state, EquipmentState::Idle);
}

TEST(MiniFabControllerTest, CompletedJobStatusPersists)
{
    SqliteRecipeRepository repository(":memory:");
    RecipeManager manager(repository);
    MiniFabController controller(manager);

    ASSERT_TRUE(controller.addRecipe({1, "Fast Etch", 0.01}));

    const auto jobId = controller.enqueueJob("W1", 1, 1);

    controller.startScheduler();

    ASSERT_TRUE(waitUntil([&]()
    {
        return controller.jobStatus(jobId) == JobStatus::Completed;
    }));

    controller.stopScheduler();

    EXPECT_EQ(controller.jobStatus(jobId), JobStatus::Completed);
    EXPECT_EQ(controller.jobStatus(999999), std::nullopt);
}

TEST(MiniFabControllerTest, StoppedSchedulerDoesNotDispatchJobs)
{
    SqliteRecipeRepository repository(":memory:");
    RecipeManager manager(repository);
    MiniFabController controller(manager);

    ASSERT_TRUE(controller.addRecipe({1, "Fast Etch", 0.01}));

    controller.startScheduler();
    controller.stopScheduler();

    const auto jobId = controller.enqueueJob("W1", 1, 1);

    EXPECT_EQ(controller.jobStatus(jobId), JobStatus::Queued);
    EXPECT_EQ(controller.pendingJobCount(), 1);
    EXPECT_EQ(controller.status().state, EquipmentState::Idle);
}

TEST(MiniFabControllerTest, InvalidJobDoesNotBlockValidJob)
{
    SqliteRecipeRepository repository(":memory:");
    RecipeManager manager(repository);
    MiniFabController controller(manager);

    ASSERT_TRUE(controller.addRecipe({1, "Fast Etch", 0.01}));

    const auto invalidJob = controller.enqueueJob("W1", 1, 2);
    const auto validJob = controller.enqueueJob("W1", 1, 1);

    controller.startScheduler();

    ASSERT_TRUE(waitUntil([&]()
    {
        return controller.jobStatus(validJob) == JobStatus::Completed;
    }));

    controller.stopScheduler();

    EXPECT_EQ(controller.jobStatus(invalidJob), JobStatus::Rejected);
    EXPECT_EQ(controller.jobStatus(validJob), JobStatus::Completed);
    EXPECT_EQ(controller.pendingJobCount(), 0);
}

TEST(MiniFabControllerTest, InvalidRecipeIsRejected)
{
    SqliteRecipeRepository repository(":memory:");
    RecipeManager manager(repository);
    MiniFabController controller(manager);

    const auto jobId = controller.enqueueJob("W1", 999, 1);

    controller.startScheduler();

    ASSERT_TRUE(waitUntil([&]()
    {
        return controller.jobStatus(jobId) == JobStatus::Rejected;
    }));

    controller.stopScheduler();

    EXPECT_EQ(controller.pendingJobCount(), 0);
}


TEST(MiniFabControllerTest, DispatchReturnsExplicitResults)
{
    SqliteRecipeRepository repository(":memory:");
    RecipeManager manager(repository);
    MiniFabController controller(manager);

    ASSERT_TRUE(controller.addRecipe({1, "Fast Etch", 0.01}));

    EXPECT_EQ(
        controller.dispatchNextJob(),
        DispatchResult::Empty
    );

    const auto invalidJob = controller.enqueueJob("W1", 1, 2);

    EXPECT_EQ(
        controller.dispatchNextJob(),
        DispatchResult::Rejected
    );

    EXPECT_EQ(
        controller.jobStatus(invalidJob),
        JobStatus::Rejected
    );

    const auto validJob = controller.enqueueJob("W1", 1, 1);

    EXPECT_EQ(
        controller.dispatchNextJob(),
        DispatchResult::Started
    );

    EXPECT_EQ(
        controller.dispatchNextJob(),
        DispatchResult::Busy
    );

    EXPECT_TRUE(controller.waitForActiveJobCompletion());

    EXPECT_EQ(
        controller.jobStatus(validJob),
        JobStatus::Completed
    );

    EXPECT_EQ(
        controller.dispatchNextJob(),
        DispatchResult::Empty
    );
}

TEST(MiniFabControllerTest, SchedulerResumesAfterBusyJobCompletes)
{
    SqliteRecipeRepository repository(":memory:");
    RecipeManager manager(repository);
    MiniFabController controller(manager);

    ASSERT_TRUE(controller.addRecipe({1, "Fast Etch", 0.01}));

    const auto job1 = controller.enqueueJob("W1", 1, 1);
    const auto job2 = controller.enqueueJob("W1", 1, 1);

    ASSERT_EQ(
        controller.dispatchNextJob(),
        DispatchResult::Started
    );

    controller.startScheduler();

    // The scheduler sees an active job and must wait.
    EXPECT_EQ(controller.jobStatus(job1), JobStatus::Running);
    EXPECT_EQ(controller.jobStatus(job2), JobStatus::Queued);

    // Only this test thread consumes job1's future.
    ASSERT_TRUE(controller.waitForActiveJobCompletion());

    // Completion notifies the autonomous scheduler.
    ASSERT_TRUE(waitUntil([&]()
    {
        return controller.jobStatus(job2) == JobStatus::Completed;
    }));

    controller.stopScheduler();

    EXPECT_EQ(controller.jobStatus(job1), JobStatus::Completed);
    EXPECT_EQ(controller.jobStatus(job2), JobStatus::Completed);
}

TEST(MiniFabControllerTest, ManualCallerCannotConsumeSchedulerJob)
{
    SqliteRecipeRepository repository(":memory:");
    RecipeManager manager(repository);
    MiniFabController controller(manager);

    ASSERT_TRUE(controller.addRecipe({1, "Fast Etch", 0.5}));

    controller.startScheduler();

    const auto jobId = controller.enqueueJob("W1", 1, 1);

    ASSERT_TRUE(waitUntil([&]()
    {
        return controller.jobStatus(jobId) == JobStatus::Running;
    }));

    // The scheduler owns this job's completion.
    EXPECT_FALSE(controller.waitForActiveJobCompletion());

    // The scheduler must still complete it normally.
    ASSERT_TRUE(waitUntil([&]()
    {
        return controller.jobStatus(jobId) == JobStatus::Completed;
    }));

    controller.stopScheduler();

    EXPECT_EQ(controller.pendingJobCount(), 0);
}

TEST(MiniFabControllerTest, ManualJobKeepsCompletionOwnership)
{
    SqliteRecipeRepository repository(":memory:");
    RecipeManager manager(repository);
    MiniFabController controller(manager);

    ASSERT_TRUE(controller.addRecipe({1, "Fast Etch", 0.01}));

    const auto job1 = controller.enqueueJob("W1", 1, 1);
    const auto job2 = controller.enqueueJob("W1", 1, 1);

    ASSERT_EQ(
        controller.dispatchNextJob(),
        DispatchResult::Started
    );

    controller.startScheduler();

    EXPECT_EQ(controller.jobStatus(job1), JobStatus::Running);

    // Manual caller owns job1.
    EXPECT_TRUE(controller.waitForActiveJobCompletion());

    ASSERT_TRUE(waitUntil([&]()
    {
        return controller.jobStatus(job2) == JobStatus::Completed;
    }));

    controller.stopScheduler();

    EXPECT_EQ(controller.jobStatus(job1), JobStatus::Completed);
    EXPECT_EQ(controller.jobStatus(job2), JobStatus::Completed);
}


TEST(MiniFabControllerTest, SchedulerInitiallyHasNoFault)
{
    MiniFabController controller;

    EXPECT_EQ(
        controller.schedulerState(),
        SchedulerState::Stopped
    );

    EXPECT_TRUE(controller.schedulerError().empty());

    EXPECT_FALSE(controller.resetSchedulerFault());
}


TEST(MiniFabControllerTest, CompletionExceptionFaultsScheduler)
{
    SqliteRecipeRepository repository(":memory:");
    RecipeManager manager(repository);

    FaultInjectingController controller(manager);

    ASSERT_TRUE(
        controller.addRecipe({1, "Fast Etch", 0.01})
    );

    const auto job1 = controller.enqueueJob("W1", 1, 1);
    const auto job2 = controller.enqueueJob("W1", 1, 1);

    controller.startScheduler();

    ASSERT_TRUE(waitUntil([&]()
    {
        return controller.schedulerState()
            == SchedulerState::Faulted;
    }));

    EXPECT_EQ(
        controller.jobStatus(job1),
        JobStatus::Failed
    );

    EXPECT_EQ(
        controller.jobStatus(job2),
        JobStatus::Queued
    );

    EXPECT_EQ(controller.pendingJobCount(), 1);

    EXPECT_EQ(
        controller.schedulerError(),
        "Injected completion failure"
    );

    controller.stopScheduler();
}

TEST(MiniFabControllerTest, SchedulerRecoversAfterCompletionException)
{
    SqliteRecipeRepository repository(":memory:");
    RecipeManager manager(repository);

    FaultInjectingController controller(manager);

    ASSERT_TRUE(
        controller.addRecipe({1, "Fast Etch", 0.01})
    );

    const auto job1 = controller.enqueueJob("W1", 1, 1);
    const auto job2 = controller.enqueueJob("W1", 1, 1);

    controller.startScheduler();

    ASSERT_TRUE(waitUntil([&]()
    {
        return controller.schedulerState()
            == SchedulerState::Faulted;
    }));

    controller.stopScheduler();

    EXPECT_EQ(controller.jobStatus(job1), JobStatus::Failed);
    EXPECT_EQ(controller.jobStatus(job2), JobStatus::Queued);

    // The equipment finished successfully before the
    // completion-boundary exception was injected.
    ASSERT_EQ(controller.status().state, EquipmentState::Idle);

    ASSERT_TRUE(controller.resetSchedulerFault());

    EXPECT_EQ(
        controller.schedulerState(),
        SchedulerState::Stopped
    );

    EXPECT_TRUE(controller.schedulerError().empty());

    // The fault injector fires only once.
    controller.startScheduler();

    ASSERT_TRUE(waitUntil([&]()
    {
        return controller.jobStatus(job2)
            == JobStatus::Completed;
    }));

    controller.stopScheduler();

    EXPECT_EQ(controller.jobStatus(job1), JobStatus::Failed);
    EXPECT_EQ(controller.jobStatus(job2), JobStatus::Completed);
    EXPECT_EQ(controller.pendingJobCount(), 0);
}


TEST(MiniFabControllerTest, InitialWaferInventory)
{
    MiniFabController controller;

    const auto wafers = controller.wafersSnapshot();

    ASSERT_EQ(wafers.size(), 1);

    EXPECT_EQ(wafers.front().id, "W1");
    EXPECT_EQ(
        wafers.front().location,
        (Location{Station::LoadPort, 3})
    );
}

TEST(MiniFabControllerTest, RegistersAdditionalWafer)
{
    MiniFabController controller;

    const Wafer newWafer{
        .id = "W2",
        .location = Location{Station::LoadPort, 4}
    };

    ASSERT_EQ(controller.registerWafer(newWafer), WaferRegistrationResult::Registered);

    const auto wafers = controller.wafersSnapshot();

    ASSERT_EQ(wafers.size(), 2);

    const auto findWafer = [&wafers](const std::string& id)
    {
        return std::find_if(
            wafers.begin(),
            wafers.end(),
            [&id](const Wafer& wafer)
            {
                return wafer.id == id;
            }
        );
    };

    const auto w1 = findWafer("W1");
    const auto w2 = findWafer("W2");

    ASSERT_NE(w1, wafers.end());
    ASSERT_NE(w2, wafers.end());

    EXPECT_EQ(w1->location, (Location{Station::LoadPort, 3}));
    EXPECT_EQ(w2->location, (Location{Station::LoadPort, 4}));
}

TEST(MiniFabControllerTest, RejectsDuplicateWaferId)
{
    MiniFabController controller;

    const Wafer duplicate{
        .id = "W1",
        .location = Location{Station::LoadPort, 4}
    };

    EXPECT_EQ(controller.registerWafer(duplicate), WaferRegistrationResult::DuplicateId);
    EXPECT_EQ(controller.wafersSnapshot().size(), 1);
}

TEST(MiniFabControllerTest, RejectsOccupiedLoadPortSlot)
{
    MiniFabController controller;

    const Wafer conflicting{
        .id = "W2",
        .location = Location{Station::LoadPort, 3}
    };

    EXPECT_EQ(controller.registerWafer(conflicting), WaferRegistrationResult::SlotOccupied);
    EXPECT_EQ(controller.wafersSnapshot().size(), 1);
}


TEST(MiniFabControllerTest, RegistrationGeneratesEvent)
{
    MiniFabController controller;

    const auto before = controller.eventsSnapshot().size();

    const auto result = controller.registerWafer(
        Wafer{"W2", {Station::LoadPort, 4}}
    );

    ASSERT_EQ(result, WaferRegistrationResult::Registered);

    const auto events = controller.eventsSnapshot();

    ASSERT_EQ(events.size(), before + 1);
    EXPECT_EQ(events.back().type, EventType::WaferRegistered);
}

TEST(MiniFabControllerTest, RejectedRegistrationDoesNotGenerateEvent)
{
    MiniFabController controller;

    const auto before = controller.eventsSnapshot().size();

    const auto result = controller.registerWafer(
        Wafer{"W1", {Station::LoadPort, 4}}
    );

    ASSERT_EQ(result, WaferRegistrationResult::DuplicateId);

    const auto events = controller.eventsSnapshot();

    EXPECT_EQ(events.size(), before);
}


