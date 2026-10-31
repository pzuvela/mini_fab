#include <gtest/gtest.h>
#include <barrier>

#include "engine/process_engine.h"
#include "engine/equipment_state.h"
#include "equipment/process_module/sim_process_module.h"
#include "mocks/mock_process_module.h"
#include "mocks/blocking_mock_process_module.h"
#include "mocks/throwing_mock_process_module.h"
#include "mocks/mock_robot.h"
#include "mocks/blocking_mock_robot.h"
#include "mocks/mock_wafer_manager.h"
#include "wafer/wafer_manager.h"
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

        OperationResult operationResult = engine.processWafer(wafer_.id, destination_, recipe_);

        EXPECT_EQ(operationResult, OperationResult::Completed);
    }

    TEST_F(ProcessEngineTest, ProcessWaferReturnsFalseWhenProcessModuleFails)
    {
        MockRobot robot{ true };
        MockProcessModule processModule{ false };
        ProcessEngine engine(mgr_, robot, processModule);

        OperationResult operationResult = engine.processWafer(wafer_.id, destination_, recipe_);

        EXPECT_EQ(operationResult, OperationResult::Failed);
    }

    TEST_F(ProcessEngineTest, DoesNotProcessWaferWhenMovementFails)
    {
        MockRobot robot{ false };
        MockProcessModule processModule{ true };
        ProcessEngine engine(mgr_, robot, processModule);

        OperationResult operationResult = engine.processWafer(wafer_.id, destination_, recipe_);

        EXPECT_EQ(operationResult, OperationResult::Failed);
        EXPECT_FALSE(processModule.processWasCalled());
        EXPECT_EQ(engine.state(), EquipmentState::Faulted);
    }

    TEST_F(ProcessEngineTest, ProcessesWaferWhenMovementSucceeds)
    {
        MockRobot robot{ true };
        MockProcessModule processModule{ true };
        ProcessEngine engine(mgr_, robot, processModule);

        OperationResult operationResult = engine.processWafer(wafer_.id, destination_, recipe_);

        EXPECT_EQ(operationResult, OperationResult::Completed);
        EXPECT_TRUE(processModule.processWasCalled());
    }

    TEST_F(ProcessEngineTest, ReturnsFalseWhenProcessingFails)
    {
        MockRobot robot{ true };
        MockProcessModule processModule{ false };
        ProcessEngine engine(mgr_, robot, processModule);

        OperationResult operationResult = engine.processWafer(wafer_.id, destination_, recipe_);

        Wafer* managedWafer = mgr_.findWafer(wafer_.id);

        EXPECT_EQ(operationResult, OperationResult::Failed);
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

        OperationResult operationResult = engine.processWafer(wafer_.id, destination_, recipe_);

        EXPECT_EQ(operationResult, OperationResult::Failed);
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

        OperationResult operationResult = engine.processWafer(wafer_.id, destination_, recipe_);

        EXPECT_EQ(operationResult, OperationResult::Completed);
        EXPECT_EQ(engine.state(), EquipmentState::Idle);
    }

    TEST_F(ProcessEngineTest, DoesNotProcessWaferWhenWaferDoesNotExist) 
    {
        MockRobot robot{ true };
        MockProcessModule processModule{ true };
        ProcessEngine engine(mgr_, robot, processModule);

        OperationResult operationResult = engine.processWafer("404", destination_, recipe_);

        EXPECT_EQ(operationResult, OperationResult::Failed);
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

        ASSERT_EQ(
            engine.startProcessWafer(wafer_.id, destination_, recipe_),
	    OperationResult::Completed
        );

        processModule.waitUntilProcessingStarts();

        EXPECT_EQ(
            engine.startProcessWafer("Wafer2", destination_, recipe_),
	    OperationResult::Failed
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

        OperationResult operationResult = engine.startProcessWafer(
            wafer_.id, destination_, recipe_
        );

        ASSERT_EQ(operationResult, OperationResult::Completed);

        operationResult = engine.waitForCompletion();

        EXPECT_EQ(operationResult, OperationResult::Completed);
        EXPECT_EQ(engine.state(), EquipmentState::Idle);
    }

    TEST_F(ProcessEngineTest, AsyncProcessWaferReturnsFailedCompletion) 
    {
        MockRobot robot{ true };
        MockProcessModule processModule{false};

        WaferManager mgr = WaferManager();

        mgr.addWafer(wafer_);

        ProcessEngine engine(mgr, robot, processModule);

        OperationResult operationResult = engine.startProcessWafer(
            wafer_.id, destination_, recipe_
        );

        ASSERT_EQ(operationResult, OperationResult::Completed);

        operationResult = engine.waitForCompletion();

        EXPECT_EQ(operationResult, OperationResult::Failed);
        EXPECT_EQ(engine.state(), EquipmentState::Faulted);
    }

    TEST_F(ProcessEngineTest, WaitForCompletionThrowsWhenCalledTwice)
    {
        MockRobot robot{ true };
        SimProcessModule processModule;

        WaferManager mgr = WaferManager();

        mgr.addWafer(wafer_);

        ProcessEngine engine(mgr, robot, processModule);

        OperationResult operationResult = engine.startProcessWafer(
            wafer_.id, destination_, recipe_
        );

        ASSERT_EQ(operationResult, OperationResult::Completed);

        operationResult = engine.waitForCompletion();

        EXPECT_EQ(operationResult, OperationResult::Completed);

        EXPECT_EQ(engine.state(), EquipmentState::Idle);

        EXPECT_THROW(
            engine.waitForCompletion(),
            std::logic_error
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

        ASSERT_EQ(
		engine.startProcessWafer(
            		wafer_.id, destination_, recipe_
        	),
		OperationResult::Completed
	);

        ASSERT_EQ(engine.waitForCompletion(), OperationResult::Completed);

        // Actual behavior under test
        EXPECT_EQ(
		engine.startProcessWafer(
            		"Wafer2", destination_, recipe_
        	),
		OperationResult::Completed
	);

        EXPECT_EQ(engine.waitForCompletion(), OperationResult::Completed);
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

        OperationResult firstStarted = OperationResult::Failed;
        OperationResult secondStarted = OperationResult::Failed;

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

        EXPECT_EQ(engine.waitForCompletion(), OperationResult::Completed);
    }

    TEST_F(ProcessEngineTest, AsyncProcessWaferPropagatesUnexpectedException) 
    {
        MockRobot robot{ true };
        ThrowingMockProcessModule processModule;

        WaferManager mgr = WaferManager();

        mgr.addWafer(wafer_);

        ProcessEngine engine(mgr, robot, processModule);

        OperationResult operationResult = engine.startProcessWafer(
            wafer_.id, destination_, recipe_
        );

        ASSERT_EQ(operationResult, OperationResult::Completed);

        EXPECT_THROW(
            engine.waitForCompletion(),
            std::runtime_error
        );

        EXPECT_EQ(engine.state(), EquipmentState::Faulted);
    }

    TEST_F(ProcessEngineTest, TransitionsToStoppingWhenCancelledDuringProcessing) 
    {
	MockRobot robot{ true };
        BlockingMockProcessModule processModule;

        WaferManager mgr = WaferManager();

        mgr.addWafer(wafer_);

        ProcessEngine engine(mgr, robot, processModule);

	engine.startProcessWafer(wafer_.id, destination_, recipe_);
	processModule.waitUntilProcessingStarts();
	StateTransition stateTransition = engine.cancel();

	processModule.release();

	EXPECT_EQ(stateTransition.previousState, EquipmentState::Processing);
	EXPECT_EQ(stateTransition.newState, EquipmentState::Stopping);
	EXPECT_TRUE(stateTransition.isCancelled);

	EXPECT_EQ(engine.state(), EquipmentState::Stopping);
    }

    TEST_F(ProcessEngineTest, RemainsIdleWhenCancelledWhileIdle) 
    {
	MockRobot robot{ true };
        BlockingMockProcessModule processModule;

        WaferManager mgr = WaferManager();

        mgr.addWafer(wafer_);

        ProcessEngine engine(mgr, robot, processModule);

        StateTransition stateTransition = engine.cancel();

        processModule.release();

        EXPECT_EQ(stateTransition.previousState, EquipmentState::Idle);
        EXPECT_EQ(stateTransition.newState, EquipmentState::Idle);
        EXPECT_FALSE(stateTransition.isCancelled);
    }

    TEST_F(ProcessEngineTest, DoesNotProcessWaferWhenCancelledDuringMovement) 
    {
	BlockingMockRobot robot;
	MockProcessModule processModule{ false };
	WaferManager mgr;
	mgr.addWafer(wafer_);

	ProcessEngine engine{mgr, robot, processModule};

	engine.startProcessWafer(
		wafer_.id, destination_, recipe_
	);

	robot.waitUntilMovementStarts();

	StateTransition stateTransition = engine.cancel();
	robot.release();

	engine.waitForCompletion();

	EXPECT_FALSE(processModule.processWasCalled());
	EXPECT_EQ(stateTransition.newState, EquipmentState::Stopping);
    }
}
