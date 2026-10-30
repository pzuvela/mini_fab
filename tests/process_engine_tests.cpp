#include <gtest/gtest.h>
#include <barrier>

#include "engine/process_engine.h"
#include "engine/equipment_state.h"
#include "equipment/process_module/sim_process_module.h"
#include "mocks/mock_process_module.h"
#include "mocks/blocking_mock_process_module.h"
#include "mocks/throwing_mock_process_module.h"
#include "mocks/mock_robot.h"
#include "mocks/mock_wafer_manager.h"
#include "wafer_manager.h"
#include "domain/wafer.h"
#include "domain/location.h"

namespace MiniFab::Tests
{
    class ProcessEngineTest : public ::testing::Test
    {
    protected:
        Wafer wafer_{
            .id = "1",
            .location = Location{
                .station = Station::LoadPort,
                .slot = 2
            }
        };

        Location destination_{
            .station = Station::Aligner,
            .slot = 7
        };

        Recipe recipe_{
            .id = 1,
            .name = "Recipe1",
            .durationSeconds = 55.2
        };

        MockWaferManager mgr_{ std::make_unique<Wafer>(wafer_) };

        SimProcessModule processModule_;
    };

    TEST_F(ProcessEngineTest, UpdatesWaferLocationWhenRobotMoveSucceeds)
    {
        MockRobot robot{ true };
        ProcessEngine engine(mgr_, robot, processModule_);

        engine.processWafer(wafer_.id, destination_, recipe_);

        Wafer* managedWafer = mgr_.findWafer(wafer_.id);

        ASSERT_NE(managedWafer, nullptr);
        EXPECT_EQ(managedWafer->location, destination_);
    }

    TEST_F(ProcessEngineTest, KeepsWaferLocationWhenRobotMoveFails)
    {
        MockRobot robot{ false };
        ProcessEngine engine(mgr_, robot, processModule_);
        Location originalLocation = wafer_.location;

        engine.processWafer(wafer_.id, destination_, recipe_);

        Wafer* managedWafer = mgr_.findWafer(wafer_.id);

        ASSERT_NE(managedWafer, nullptr);
        EXPECT_EQ(managedWafer->location, originalLocation);
    }

    TEST_F(ProcessEngineTest, ProcessWaferReturnsTrueWhenProcessModuleSucceeds)
    {
        MockRobot robot{ true };
        MockProcessModule processModule{ true };
        ProcessEngine engine(mgr_, robot, processModule);

        bool isSuccessful = engine.processWafer(wafer_.id, destination_, recipe_);

        EXPECT_TRUE(isSuccessful);
    }

    TEST_F(ProcessEngineTest, ProcessWaferReturnsFalseWhenProcessModuleFails)
    {
        MockRobot robot{ true };
        MockProcessModule processModule{ false };
        ProcessEngine engine(mgr_, robot, processModule);

        bool isSuccessful = engine.processWafer(wafer_.id, destination_, recipe_);

        EXPECT_FALSE(isSuccessful);
    }

    TEST_F(ProcessEngineTest, DoesNotProcessWaferWhenMovementFails)
    {
        MockRobot robot{ false };
        MockProcessModule processModule{ true };
        ProcessEngine engine(mgr_, robot, processModule);

        bool isProcessed = engine.processWafer(wafer_.id, destination_, recipe_);

        EXPECT_FALSE(isProcessed);
        EXPECT_FALSE(processModule.processWasCalled());
        EXPECT_EQ(engine.state(), EquipmentState::Faulted);
    }

    TEST_F(ProcessEngineTest, ProcessesWaferWhenMovementSucceeds)
    {
        MockRobot robot{ true };
        MockProcessModule processModule{ true };
        ProcessEngine engine(mgr_, robot, processModule);

        bool isProcessed = engine.processWafer(wafer_.id, destination_, recipe_);

        EXPECT_TRUE(isProcessed);
        EXPECT_TRUE(processModule.processWasCalled());
    }

    TEST_F(ProcessEngineTest, ReturnsFalseWhenProcessingFails)
    {
        MockRobot robot{ true };
        MockProcessModule processModule{ false };
        ProcessEngine engine(mgr_, robot, processModule);

        bool isProcessed = engine.processWafer(wafer_.id, destination_, recipe_);

        Wafer* managedWafer = mgr_.findWafer(wafer_.id);

        EXPECT_FALSE(isProcessed);
        EXPECT_TRUE(processModule.processWasCalled());
        ASSERT_NE(managedWafer, nullptr);
        EXPECT_EQ(destination_, managedWafer->location);
    }

    TEST_F(ProcessEngineTest, IsIdleWhenCreated)
    {
        MockRobot robot{ true };
        MockProcessModule processModule{ true };
        ProcessEngine engine(mgr_, robot, processModule);
        
        EXPECT_EQ(engine.state(), EquipmentState::Idle);
    }

    TEST_F(ProcessEngineTest, DoesNotProcessWaferWhenEngineIsFaulted)
    {
        MockRobot robot{ false };
        MockProcessModule processModule{ true };
        ProcessEngine engine(mgr_, robot, processModule);

        engine.processWafer(wafer_.id, destination_, recipe_);
        robot.setMoveShouldSucceed(true);

        bool isProcessed = engine.processWafer(wafer_.id, destination_, recipe_);

        EXPECT_FALSE(isProcessed);
        EXPECT_EQ(engine.state(), EquipmentState::Faulted);
        EXPECT_FALSE(processModule.processWasCalled());
    }

    TEST_F(ProcessEngineTest, ResetReturnsEngineToIdleAfterFault) 
    {
        MockRobot robot{ false };
        MockProcessModule processModule{ true };
        ProcessEngine engine(mgr_, robot, processModule);

        engine.processWafer(wafer_.id, destination_, recipe_);

        bool isResetSuccessful = engine.reset();

        EXPECT_TRUE(isResetSuccessful);
        EXPECT_EQ(engine.state(), EquipmentState::Idle);
    }

    TEST_F(ProcessEngineTest, ResetFailsWhenEngineIsNotFaulted) 
    {
        MockRobot robot{ false };
        MockProcessModule processModule{ true };
        ProcessEngine engine(mgr_, robot, processModule);

        bool isResetSuccessful = engine.reset();

        EXPECT_FALSE(isResetSuccessful);
        EXPECT_EQ(engine.state(), EquipmentState::Idle);
    }

    TEST_F(ProcessEngineTest, ProcessesWaferSuccessfullyAfterResettingFromFault) 
    {
        MockRobot robot{ false };
        MockProcessModule processModule{ true };
        ProcessEngine engine(mgr_, robot, processModule);

        engine.processWafer(wafer_.id, destination_, recipe_);
        
        engine.reset();
        
        robot.setMoveShouldSucceed(true);

        bool isProcessed = engine.processWafer(wafer_.id, destination_, recipe_);

        EXPECT_TRUE(isProcessed);
        EXPECT_EQ(engine.state(), EquipmentState::Idle);
    }

    TEST_F(ProcessEngineTest, DoesNotProcessWaferWhenWaferDoesNotExist) 
    {
        MockRobot robot{ true };
        MockProcessModule processModule{ true };
        ProcessEngine engine(mgr_, robot, processModule);

        bool isProcessed = engine.processWafer("404", destination_, recipe_);

        EXPECT_FALSE(isProcessed);
        EXPECT_FALSE(processModule.processWasCalled());
        EXPECT_EQ(engine.state(), EquipmentState::Idle);
    }

    TEST_F(ProcessEngineTest, RejectsSecondWaferWhileAsyncProcessingIsActive) 
    {
        MockRobot robot{ true };
        BlockingMockProcessModule processModule;

        WaferManager mgr = WaferManager();

        mgr.addWafer(wafer_);

        ProcessEngine engine(mgr, robot, processModule);

        mgr.addWafer(
            Wafer{
                .id = "Wafer2",
                .location = Location{
                    .station = Station::Aligner,
                    .slot = 2
                }
            }
        );

        ASSERT_TRUE(
            engine.startProcessWafer(wafer_.id, destination_, recipe_)
        );

        processModule.waitUntilProcessingStarts();

        EXPECT_FALSE(
            engine.startProcessWafer("Wafer2", destination_, recipe_)
        );

        processModule.release();
    }

    TEST_F(ProcessEngineTest, AsyncProcessWaferReturnsSuccessfulCompletion)
    {
        MockRobot robot{ true };
        SimProcessModule processModule;

        WaferManager mgr = WaferManager();

        mgr.addWafer(wafer_);

        ProcessEngine engine(mgr, robot, processModule);

        bool started = engine.startProcessWafer(
            wafer_.id, destination_, recipe_
        );

        ASSERT_TRUE(started);

        bool completedSuccessfully = engine.waitForCompletion();

        EXPECT_TRUE(completedSuccessfully);
        EXPECT_EQ(engine.state(), EquipmentState::Idle);
    }

    TEST_F(ProcessEngineTest, AsyncProcessWaferReturnsFailedCompletion) 
    {
        MockRobot robot{ true };
        MockProcessModule processModule{false};

        WaferManager mgr = WaferManager();

        mgr.addWafer(wafer_);

        ProcessEngine engine(mgr, robot, processModule);

        bool started = engine.startProcessWafer(
            wafer_.id, destination_, recipe_
        );

        ASSERT_TRUE(started);

        bool completedSuccessfully = engine.waitForCompletion();

        EXPECT_FALSE(completedSuccessfully);
        EXPECT_EQ(engine.state(), EquipmentState::Faulted);
    }

    TEST_F(ProcessEngineTest, WaitForCompletionThrowsWhenCalledTwice)
    {
        MockRobot robot{ true };
        SimProcessModule processModule;

        WaferManager mgr = WaferManager();

        mgr.addWafer(wafer_);

        ProcessEngine engine(mgr, robot, processModule);

        bool started = engine.startProcessWafer(
            wafer_.id, destination_, recipe_
        );

        ASSERT_TRUE(started);

        bool completedSuccessfully = engine.waitForCompletion();

        EXPECT_TRUE(completedSuccessfully);

        EXPECT_EQ(engine.state(), EquipmentState::Idle);

        EXPECT_THROW(
            engine.waitForCompletion(),
            std::future_error
        );
    }

    TEST_F(ProcessEngineTest, CanProcessAnotherWaferAfterAsyncCompletion) 
    {
        MockRobot robot{ true };
        SimProcessModule processModule;

        WaferManager mgr = WaferManager();

        mgr.addWafer(wafer_);
        mgr.addWafer(
            Wafer{
                .id = "Wafer2",
                .location = Location{
                    .station = Station::Aligner,
                    .slot = 2
                }
            }
        );

        ProcessEngine engine(mgr, robot, processModule);

        ASSERT_TRUE(engine.startProcessWafer(
            wafer_.id, destination_, recipe_
        ));

        ASSERT_TRUE(engine.waitForCompletion());

        // Actual behavior under test
        EXPECT_TRUE(engine.startProcessWafer(
            "Wafer2", destination_, recipe_
        ));

        EXPECT_TRUE(engine.waitForCompletion());
        EXPECT_EQ(engine.state(), EquipmentState::Idle);
    }

    TEST_F(ProcessEngineTest, AcceptsOnlyOneOfTwoSimultaneousStartRequests)
    {
        MockRobot robot{ true };
        BlockingMockProcessModule processModule;

        WaferManager mgr = WaferManager();

        mgr.addWafer(wafer_);
        mgr.addWafer(
            Wafer{
                .id = "Wafer2",
                .location = Location{
                    .station = Station::Aligner,
                    .slot = 2
                }
            }
        );

        ProcessEngine engine(mgr, robot, processModule);

        std::barrier startBarrier{ 2 };

        bool firstStarted = false;
        bool secondStarted = false;

        std::thread firstThread([&]() {
            startBarrier.arrive_and_wait();

            firstStarted = engine.startProcessWafer(
                wafer_.id, destination_, recipe_
            );
        });

        std::thread secondThread([&]() {
            startBarrier.arrive_and_wait();

            secondStarted = engine.startProcessWafer(
                "Wafer2", destination_, recipe_
            );
        });

        firstThread.join();
        secondThread.join();

        processModule.release();

        EXPECT_NE(firstStarted, secondStarted);

        EXPECT_TRUE(engine.waitForCompletion());
    }

    TEST_F(ProcessEngineTest, AsyncProcessWaferPropagatesUnexpectedException) 
    {
        MockRobot robot{ true };
        ThrowingMockProcessModule processModule;

        WaferManager mgr = WaferManager();

        mgr.addWafer(wafer_);

        ProcessEngine engine(mgr, robot, processModule);

        bool started = engine.startProcessWafer(
            wafer_.id, destination_, recipe_
        );

        ASSERT_TRUE(started);        

        EXPECT_THROW(
            engine.waitForCompletion(),
            std::runtime_error
        );

        EXPECT_EQ(engine.state(), EquipmentState::Faulted);
    }
}
