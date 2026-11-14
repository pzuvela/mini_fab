#include <gtest/gtest.h>
#include <barrier>
#include <algorithm>
#include <future>
#include <chrono>
#include <thread>

#include "engine/process_engine.h"
#include "engine/equipment_state.h"
#include "engine/start_result.h"
#include "equipment/process_module/sim_process_module.h"
#include "mocks/mock_process_module.h"
#include "mocks/blocking_mock_process_module.h"
#include "mocks/throwing_mock_process_module.h"
#include "mocks/mock_robot.h"
#include "mocks/blocking_mock_robot.h"
#include "mocks/two_stage_blocking_mock_robot.h"
#include "mocks/mock_wafer_manager.h"
#include "wafer/wafer_manager.h"
#include "domain/wafer.h"
#include "domain/location.h"
#include "equipment/event_log.h"

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
            .slot = 1
        };

        Recipe recipe_{
            .id = 1,
            .name = "Recipe1",
            .durationSeconds = 0.05
        };

        Recipe longRecipe_{
            .id = 5,
            .name = "Recipe2",
            .durationSeconds = 60
        };

        MockWaferManager mgr_{ std::make_unique<Wafer>(wafer_), true };

        SimProcessModule processModule_;

	EventLog eventLog_ = EventLog( {} );
    };

    TEST_F(ProcessEngineTest, RequestsLocationUpdateWhenRobotMoveSucceeds)
    {
        MockRobot robot{ true };
        ProcessEngine engine(mgr_, robot, processModule_, eventLog_);

        engine.processWafer(wafer_.id, destination_, recipe_);

        // EXPECT_EQ(managedWafer->location, destination_);  MockWaferManager does not update the location!
        EXPECT_TRUE(mgr_.locationUpdateWasCalled());
    }

    TEST_F(ProcessEngineTest, DoesNotRequestLocationUpdateWhenRobotMoveFails)
    {
        MockRobot robot{ false };
        ProcessEngine engine(mgr_, robot, processModule_, eventLog_);

        engine.processWafer(wafer_.id, destination_, recipe_);

        EXPECT_FALSE(mgr_.locationUpdateWasCalled());
    }

    TEST_F(ProcessEngineTest, ProcessWaferReturnsTrueWhenProcessModuleSucceeds)
    {
        MockRobot robot{ true };
        MockProcessModule processModule{ true };
        ProcessEngine engine(mgr_, robot, processModule, eventLog_);

        OperationResult operationResult = engine.processWafer(wafer_.id, destination_, recipe_);

        EXPECT_EQ(operationResult, OperationResult::Completed);
    }

    TEST_F(ProcessEngineTest, ProcessWaferReturnsFalseWhenProcessModuleFails)
    {
        MockRobot robot{ true };
        MockProcessModule processModule{ false };
        ProcessEngine engine(mgr_, robot, processModule, eventLog_);

        OperationResult operationResult = engine.processWafer(wafer_.id, destination_, recipe_);

        EXPECT_EQ(operationResult, OperationResult::Failed);
    }

    TEST_F(ProcessEngineTest, DoesNotProcessWaferWhenMovementFails)
    {
        MockRobot robot{ false };
        MockProcessModule processModule{ true };
        ProcessEngine engine(mgr_, robot, processModule, eventLog_);

        OperationResult operationResult = engine.processWafer(wafer_.id, destination_, recipe_);

        EXPECT_EQ(operationResult, OperationResult::Failed);
        EXPECT_FALSE(processModule.processWasCalled());
        EXPECT_EQ(engine.state(), EquipmentState::Faulted);
    }

    TEST_F(ProcessEngineTest, ProcessesWaferWhenMovementSucceeds)
    {
        MockRobot robot{ true };
        MockProcessModule processModule{ true };
        ProcessEngine engine(mgr_, robot, processModule, eventLog_);

        OperationResult operationResult = engine.processWafer(wafer_.id, destination_, recipe_);

        EXPECT_EQ(operationResult, OperationResult::Completed);
        EXPECT_TRUE(processModule.processWasCalled());
    }

    TEST_F(ProcessEngineTest, ReturnsFailedWhenWaferLocationUpdateFails)
    {
        MockRobot robot{ true };
        MockProcessModule processModule{ false };
        MockWaferManager mgr{ std::make_unique<Wafer>(wafer_), false };
        ProcessEngine engine(mgr, robot, processModule, eventLog_);

        OperationResult operationResult = engine.processWafer(wafer_.id, destination_, recipe_);

        EXPECT_EQ(operationResult, OperationResult::Failed);
        EXPECT_FALSE(processModule.processWasCalled());
        EXPECT_EQ(engine.state(), EquipmentState::Faulted);
        // EXPECT_EQ(destination_, managedWafer->location); MockWaferManager doesn't actually move wafer
    }

    TEST_F(ProcessEngineTest, IsIdleWhenCreated)
    {
        MockRobot robot{ true };
        MockProcessModule processModule{ true };
        ProcessEngine engine(mgr_, robot, processModule, eventLog_);

        EXPECT_EQ(engine.state(), EquipmentState::Idle);
    }

    TEST_F(ProcessEngineTest, DoesNotProcessWaferWhenEngineIsFaulted)
    {
        MockRobot robot{ false };
        MockProcessModule processModule{ true };
        ProcessEngine engine(mgr_, robot, processModule, eventLog_);

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
        ProcessEngine engine(mgr_, robot, processModule, eventLog_);

        engine.processWafer(wafer_.id, destination_, recipe_);

        RecoveryResult recoveryResult = engine.reset();

        EXPECT_EQ(recoveryResult, RecoveryResult::Recovered);
        EXPECT_EQ(engine.state(), EquipmentState::Idle);
    }

    TEST_F(ProcessEngineTest, ResetReturnsNotRequiredWhenEngineIsNotFaulted)
    {
        MockRobot robot{ false };
        MockProcessModule processModule{ true };
        ProcessEngine engine(mgr_, robot, processModule, eventLog_);

        RecoveryResult recoveryResult = engine.reset();

        EXPECT_EQ(recoveryResult, RecoveryResult::NotRequired);
        EXPECT_EQ(engine.state(), EquipmentState::Idle);
    }

    TEST_F(ProcessEngineTest, ProcessesWaferSuccessfullyAfterResettingFromFault) 
    {
        MockRobot robot{ false };
        MockProcessModule processModule{ true };
        ProcessEngine engine(mgr_, robot, processModule, eventLog_);

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
        ProcessEngine engine(mgr_, robot, processModule, eventLog_);

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

        ProcessEngine engine(mgr, robot, processModule, eventLog_);

        mgr.addWafer(
            Wafer{
                .id = "Wafer2",
                .location = Location{
                    .station = Station::LoadPort,
                    .slot = 2
                }
            }
        );

        ASSERT_EQ(
            engine.startProcessWafer(wafer_.id, destination_, recipe_),
	    StartResult::Accepted
        );

        processModule.waitUntilProcessingStarts();

        EXPECT_EQ(
            engine.startProcessWafer("Wafer2", destination_, recipe_),
	    StartResult::Rejected
        );

        processModule.release();
    }

    TEST_F(ProcessEngineTest, AsyncProcessWaferReturnsSuccessfulCompletion)
    {
        MockRobot robot{ true };
        SimProcessModule processModule;

        WaferManager mgr = WaferManager();

        mgr.addWafer(wafer_);

        ProcessEngine engine(mgr, robot, processModule, eventLog_);

        StartResult startResult = engine.startProcessWafer(
            wafer_.id, destination_, recipe_
        );

        ASSERT_EQ(startResult, StartResult::Accepted);

        OperationResult operationResult = engine.waitForCompletion();

        EXPECT_EQ(operationResult, OperationResult::Completed);
        EXPECT_EQ(engine.state(), EquipmentState::Idle);
    }

    TEST_F(ProcessEngineTest, AsyncProcessWaferReturnsFailedCompletion)
    {
        MockRobot robot{ true };
        MockProcessModule processModule{false};

        WaferManager mgr = WaferManager();

        mgr.addWafer(wafer_);

        ProcessEngine engine(mgr, robot, processModule, eventLog_);

        StartResult startResult = engine.startProcessWafer(
            wafer_.id, destination_, recipe_
        );

        ASSERT_EQ(startResult, StartResult::Accepted);

        OperationResult operationResult = engine.waitForCompletion();

        EXPECT_EQ(operationResult, OperationResult::Failed);
        EXPECT_EQ(engine.state(), EquipmentState::Faulted);
    }

    TEST_F(ProcessEngineTest, WaitForCompletionThrowsWhenCalledTwice)
    {
        MockRobot robot{ true };
        SimProcessModule processModule;

        WaferManager mgr = WaferManager();

        mgr.addWafer(wafer_);

        ProcessEngine engine(mgr, robot, processModule, eventLog_);

        StartResult startResult = engine.startProcessWafer(
            wafer_.id, destination_, recipe_
        );

        ASSERT_EQ(startResult, StartResult::Accepted);

        OperationResult operationResult = engine.waitForCompletion();

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
                    .station = Station::LoadPort,
                    .slot = 3
                }
            }
        );

        ProcessEngine engine(mgr, robot, processModule, eventLog_);

        ASSERT_EQ(
		engine.startProcessWafer(
            		wafer_.id, destination_, recipe_
        	),
		StartResult::Accepted
	);

        ASSERT_EQ(engine.waitForCompletion(), OperationResult::Completed);

        // Actual behavior under test
        EXPECT_EQ(
		engine.startProcessWafer(
            		"Wafer2", destination_, recipe_
        	),
		StartResult::Accepted
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
                    .station = Station::LoadPort,
                    .slot = 2
                }
            }
        );

        ProcessEngine engine(mgr, robot, processModule, eventLog_);

        std::barrier startBarrier{ 2 };

        StartResult firstStarted = StartResult::Rejected;
        StartResult secondStarted = StartResult::Rejected;

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

        ProcessEngine engine(mgr, robot, processModule, eventLog_);

        StartResult startResult = engine.startProcessWafer(
            wafer_.id, destination_, recipe_
        );

        ASSERT_EQ(startResult, StartResult::Accepted);

        EXPECT_THROW(
            engine.waitForCompletion(),
            std::runtime_error
        );

        EXPECT_EQ(engine.state(), EquipmentState::Faulted);
    }

    TEST_F(ProcessEngineTest, CancelsProcessingAndReturnsToIdle) 
    {
	MockRobot robot{ true };
        BlockingMockProcessModule processModule;

        WaferManager mgr = WaferManager();

        mgr.addWafer(wafer_);

        ProcessEngine engine(mgr, robot, processModule, eventLog_);

	    engine.startProcessWafer(wafer_.id, destination_, recipe_);

	    processModule.waitUntilProcessingStarts();

	    StateTransition stateTransition = engine.cancel();

	    OperationResult operationResult = engine.waitForCompletion();

	    EXPECT_EQ(operationResult, OperationResult::Cancelled);

	    processModule.release();

	    EXPECT_EQ(stateTransition.previousState, EquipmentState::Processing);
	    EXPECT_EQ(stateTransition.newState, EquipmentState::Stopping);
	    EXPECT_TRUE(stateTransition.isCancelled);

	    EXPECT_EQ(engine.state(), EquipmentState::Idle);
    }

    TEST_F(ProcessEngineTest, RemainsIdleWhenCancelledWhileIdle)
    {
	MockRobot robot{ true };
        BlockingMockProcessModule processModule;

        WaferManager mgr = WaferManager();

        mgr.addWafer(wafer_);

        ProcessEngine engine(mgr, robot, processModule, eventLog_);

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

	    ProcessEngine engine{mgr, robot, processModule, eventLog_};

	    engine.startProcessWafer(
		    wafer_.id, destination_, recipe_
	    );

	    robot.waitUntilMovementStarts();

	    StateTransition stateTransition = engine.cancel();

	    OperationResult operationResult = engine.waitForCompletion();

	    EXPECT_EQ(operationResult, OperationResult::Cancelled);
	    EXPECT_FALSE(processModule.processWasCalled());
	    EXPECT_EQ(stateTransition.newState, EquipmentState::Stopping);
	    EXPECT_EQ(engine.state(), EquipmentState::Idle);
    }

    TEST_F(ProcessEngineTest, RecoversRobotAndReturnsToIdleWhenRobotCausedFault)
    {
	    MockRobot robot{ false };
	    MockProcessModule processModule{ true };
	    ProcessEngine engine(mgr_, robot, processModule, eventLog_);

	    engine.processWafer(wafer_.id, destination_, recipe_);

	    RecoveryResult recoveryResult = engine.reset();

	    EXPECT_EQ(recoveryResult, RecoveryResult::Recovered);
	    EXPECT_EQ(engine.state(), EquipmentState::Idle);
    }

    TEST_F(ProcessEngineTest, RecoveryFailureKeepsEngineFaulted)
    {
	    MockRobot robot { false, false };
	    MockProcessModule processModule{ true };
	    ProcessEngine engine(mgr_, robot, processModule, eventLog_);

	    engine.processWafer(wafer_.id, destination_, recipe_);

	    RecoveryResult recoveryResult = engine.reset();

	    EXPECT_EQ(recoveryResult, RecoveryResult::Failed);
	    EXPECT_EQ(engine.state(), EquipmentState::Faulted);
    }

    TEST_F(ProcessEngineTest, CanRetryRecoveryAfterRecoveryFailure)
    {
	    MockRobot robot { false, false };
	    MockProcessModule processModule{ true };
	    ProcessEngine engine(mgr_, robot, processModule, eventLog_);

	    engine.processWafer(wafer_.id, destination_, recipe_);

	    RecoveryResult recoveryResult = engine.reset();

	    EXPECT_EQ(recoveryResult, RecoveryResult::Failed);
	    EXPECT_EQ(engine.state(), EquipmentState::Faulted);

	    robot.setRecoveryShouldSucceed(true);

	    recoveryResult = engine.reset();

	    EXPECT_EQ(recoveryResult, RecoveryResult::Recovered);
	    EXPECT_EQ(engine.state(), EquipmentState::Idle);
    }

    TEST_F(ProcessEngineTest, RecoversOnlyRobotWhenRobotCausedFault)
    {
	    MockRobot robot { false, true };
	    MockProcessModule processModule { false, false };
	    ProcessEngine engine(mgr_, robot, processModule, eventLog_);

	    engine.processWafer(wafer_.id, destination_, recipe_);
	    RecoveryResult recoveryResult = engine.reset();

	    EXPECT_EQ(recoveryResult, RecoveryResult::Recovered);
	    EXPECT_FALSE(processModule.recoverWasCalled());
	    EXPECT_TRUE(robot.recoverWasCalled());
    }

    TEST_F(ProcessEngineTest, RecoversOnlyProcessModuleWhenProcessModuleCausedFault)
    {
	    MockRobot robot { true, true };
	    MockProcessModule processModule { false, true };
	    ProcessEngine engine(mgr_, robot, processModule, eventLog_);

	    engine.processWafer(wafer_.id, destination_, recipe_);
	    RecoveryResult recoveryResult = engine.reset();

	    EXPECT_EQ(recoveryResult, RecoveryResult::Recovered);
	    EXPECT_TRUE(processModule.recoverWasCalled());
	    EXPECT_FALSE(robot.recoverWasCalled());
    }

    TEST_F(ProcessEngineTest, DoesNotRemainStoppingWhenReturnMoveCompletesDuringCancellation)
    {
	TwoStageBlockingMockRobot robot;
	MockProcessModule processModule{ true };

	WaferManager mgr;
	mgr.addWafer(wafer_);

	ProcessEngine engine(mgr, robot, processModule, eventLog_);

	ASSERT_EQ(
		engine.startProcessWafer(wafer_.id, destination_, recipe_),
		StartResult::Accepted
	);

	robot.waitUntilReturnMovementStarts();

	engine.cancel();

	engine.waitForCompletion();

	EXPECT_NE(engine.state(), EquipmentState::Stopping);
    }

	TEST_F(ProcessEngineTest, NonExistentWaferDoesNotGenerateRunAcceptedEvent)
	{
		const std::string waferId = "UnknownWaferId";

		MockRobot robot { true, true };
		MockProcessModule processModule { true, true  };
		ProcessEngine engine{mgr_, robot, processModule, eventLog_};

		OperationResult result = engine.processWafer(waferId, destination_, recipe_);

		EXPECT_EQ(result, OperationResult::Failed);
		EXPECT_EQ(engine.state(), EquipmentState::Idle);

		std::vector<EquipmentEvent> events = eventLog_.getEvents();

		EXPECT_TRUE(std::ranges::none_of(
			events,
			[](const EquipmentEvent& event) {
				return event.type == EventType::RunAccepted;
			}
		));
	}

	TEST_F(ProcessEngineTest, ValidWaferGeneratesRunAcceptedEvent)
	{
		const std::string waferId = "1";

		MockRobot robot { true, true };
		MockProcessModule processModule { true, true  };
		ProcessEngine engine{mgr_, robot, processModule, eventLog_};

		OperationResult result = engine.processWafer(waferId, destination_, recipe_);

		EXPECT_EQ(result, OperationResult::Completed);
		EXPECT_EQ(engine.state(), EquipmentState::Idle);

		std::vector<EquipmentEvent> events = eventLog_.getEvents();

		EXPECT_TRUE(std::ranges::any_of(
			events,
			[](const EquipmentEvent& event) {
				return event.type == EventType::RunAccepted;
			}
		));
	}

	TEST_F(ProcessEngineTest, ReadyRobotDoesNotGenerateInitializationEvents)
	{
		const std::string waferId = "1";

		MockRobot robot { true, true };
		robot.initialize();
		MockProcessModule processModule { true, true  };
		ProcessEngine engine{mgr_, robot, processModule, eventLog_};

		OperationResult result = engine.processWafer(waferId, destination_, recipe_);

		EXPECT_EQ(result, OperationResult::Completed);
		EXPECT_EQ(engine.state(), EquipmentState::Idle);

		std::vector<EquipmentEvent> events = eventLog_.getEvents();

		EXPECT_TRUE(std::ranges::none_of(
			events,
			[](const EquipmentEvent& event) {
				return event.type == EventType::RobotInitializing || event.type == EventType::RobotReady;
			}
		));
	}


	TEST_F(ProcessEngineTest, UnitializedRobotGeneratesInitializationEventsInCorrectOrder)
	{
		const std::string waferId = "1";

		MockRobot robot { true, true };
		MockProcessModule processModule { true, true  };
		ProcessEngine engine{mgr_, robot, processModule, eventLog_};

		OperationResult result = engine.processWafer(waferId, destination_, recipe_);

		EXPECT_EQ(result, OperationResult::Completed);
		EXPECT_EQ(engine.state(), EquipmentState::Idle);

		std::vector<EquipmentEvent> events = eventLog_.getEvents();


		const EquipmentEvent* initializing = nullptr;
		const EquipmentEvent* ready = nullptr;

		for (const EquipmentEvent& event : events)
		{
			if (event.type == EventType::RobotInitializing)
				initializing = &event;

			if (event.type == EventType::RobotReady)
				ready = &event;

			if (initializing && ready)
				break;
		}

		ASSERT_NE(initializing, nullptr);
		ASSERT_NE(ready, nullptr);
		EXPECT_LT(initializing->sequenceId, ready->sequenceId);
	}

	TEST_F(ProcessEngineTest, SuccessfulProcessGeneratesProcessCompletedEvent)
	{
		const std::string waferId = "1";

		MockRobot robot { true, true };
		MockProcessModule processModule { true, true  };
		ProcessEngine engine{mgr_, robot, processModule, eventLog_};

		OperationResult result = engine.processWafer(waferId, destination_, recipe_);

		EXPECT_EQ(result, OperationResult::Completed);
		EXPECT_EQ(engine.state(), EquipmentState::Idle);

		std::vector<EquipmentEvent> events = eventLog_.getEvents();

                EXPECT_TRUE(std::ranges::any_of(
                        events,
                        [](const EquipmentEvent& event) {
                                return event.type == EventType::ProcessCompleted;
                        }
                ));
	}

	TEST_F(ProcessEngineTest, FailedProcessGeneratesProcessFailedEvent)
	{
		const std::string waferId = "1";

		MockRobot robot { true, true };
		MockProcessModule processModule { false, false  };
		ProcessEngine engine{mgr_, robot, processModule, eventLog_};

		OperationResult result = engine.processWafer(waferId, destination_, recipe_);

		EXPECT_EQ(result, OperationResult::Failed);
		EXPECT_EQ(engine.state(), EquipmentState::Faulted);

		std::vector<EquipmentEvent> events = eventLog_.getEvents();

                EXPECT_TRUE(std::ranges::any_of(
                        events,
                        [](const EquipmentEvent& event) {
                                return event.type == EventType::ProcessFailed;
                        }
                ));
	}

	TEST_F(ProcessEngineTest, CancelledProcessGeneratesProcessCancelledEvent)
	{
		const std::string waferId = "1";

		MockRobot robot { true, true };
		BlockingMockProcessModule processModule;
		ProcessEngine engine{mgr_, robot, processModule, eventLog_};

		engine.startProcessWafer(waferId, destination_, longRecipe_);

		processModule.waitUntilProcessingStarts();

		engine.cancel();

		OperationResult result = engine.waitForCompletion();

		EXPECT_EQ(result, OperationResult::Cancelled);
		EXPECT_EQ(engine.state(), EquipmentState::Idle);

		std::vector<EquipmentEvent> events = eventLog_.getEvents();

                EXPECT_TRUE(std::ranges::any_of(
                        events,
                        [](const EquipmentEvent& event) {
                                return event.type == EventType::ProcessCancelled;
                        }
                ));
	}

TEST_F(ProcessEngineTest, RecordsSuccessfulWaferLifecycleInOrder)
{
    MockRobot robot{ true };
    MockProcessModule processModule{ true };

    ProcessEngine engine(
        mgr_, robot, processModule, eventLog_
    );

    ASSERT_EQ(
        engine.processWafer(wafer_.id, destination_, recipe_),
        OperationResult::Completed
    );

    const auto events = eventLog_.getEvents();

    const std::vector<EventType> expected{
        EventType::RunAccepted,
        EventType::EquipmentMoving,
        EventType::WaferLocationUpdated,
        EventType::ProcessStarted,
        EventType::ProcessCompleted,
        EventType::EquipmentMoving,
        EventType::WaferLocationUpdated
    };

    std::size_t nextExpected = 0;

    for (const auto& event : events)
    {
        if (nextExpected < expected.size() &&
            event.type == expected[nextExpected])
        {
            ++nextExpected;
        }
    }

    EXPECT_EQ(nextExpected, expected.size());
}

TEST_F(ProcessEngineTest, DoesNotRecordProcessingWhenRobotMoveFails)
{
    MockRobot robot{ false };
    MockProcessModule processModule{ true };

    ProcessEngine engine(
        mgr_, robot, processModule, eventLog_
    );

    ASSERT_EQ(
        engine.processWafer(wafer_.id, destination_, recipe_),
        OperationResult::Failed
    );

    const auto events = eventLog_.getEvents();

    const auto hasEvent = [&](EventType type)
    {
        return std::any_of(
            events.begin(),
            events.end(),
            [type](const EquipmentEvent& event)
            {
                return event.type == type;
            }
        );
    };

    EXPECT_TRUE(hasEvent(EventType::EquipmentMoving));
    EXPECT_FALSE(hasEvent(EventType::WaferLocationUpdated));
    EXPECT_FALSE(hasEvent(EventType::ProcessStarted));
    EXPECT_FALSE(hasEvent(EventType::ProcessCompleted));

    EXPECT_EQ(engine.state(), EquipmentState::Faulted);
}


TEST_F(ProcessEngineTest,
    CanCancelWhileAnotherThreadWaitsForCompletion)
{
    MockRobot robot{true};
    BlockingMockProcessModule processModule;

    WaferManager mgr;
    mgr.addWafer(wafer_);

    ProcessEngine engine(
        mgr, robot, processModule, eventLog_
    );

    ASSERT_EQ(
        engine.startProcessWafer(
            wafer_.id, destination_, recipe_
        ),
        StartResult::Accepted
    );

    processModule.waitUntilProcessingStarts();

    // Simulate a caller waiting for the wafer result.
    auto completion = std::async(
        std::launch::async,
        [&engine]()
        {
            return engine.waitForCompletion();
        }
    );

    // Cancellation must not require the waiting
    // caller to finish first.
    StateTransition transition = engine.cancel();

    EXPECT_TRUE(transition.isCancelled);

    EXPECT_EQ(
        completion.get(),
        OperationResult::Cancelled
    );

    EXPECT_EQ(engine.state(), EquipmentState::Idle);
}


TEST_F(ProcessEngineTest,
    CompletionFutureCanOnlyBeConsumedOnce)
{
    MockRobot robot{true};
    BlockingMockProcessModule processModule;

    WaferManager mgr;
    mgr.addWafer(wafer_);

    ProcessEngine engine(
        mgr, robot, processModule, eventLog_
    );

    ASSERT_EQ(
        engine.startProcessWafer(
            wafer_.id, destination_, recipe_
        ),
        StartResult::Accepted
    );

    processModule.waitUntilProcessingStarts();

    std::barrier startBarrier{2};

    auto waitForResult = [&]()
    {
        startBarrier.arrive_and_wait();

        try
        {
            return engine.waitForCompletion();
        }
        catch (const std::logic_error&)
        {
            return OperationResult::Failed;
        }
    };

    auto first = std::async(
        std::launch::async, waitForResult
    );

    auto second = std::async(
        std::launch::async, waitForResult
    );

    processModule.release();

    OperationResult result1 = first.get();
    OperationResult result2 = second.get();

    EXPECT_TRUE(
        (result1 == OperationResult::Completed &&
         result2 == OperationResult::Failed) ||
        (result1 == OperationResult::Failed &&
         result2 == OperationResult::Completed)
    );
}


TEST_F(ProcessEngineTest,
    CanStartNextRunWithoutWaitingForPreviousFuture)
{
    MockRobot robot{true};
    SimProcessModule processModule;

    WaferManager mgr;
    mgr.addWafer(wafer_);

    ProcessEngine engine(
        mgr, robot, processModule, eventLog_
    );

    ASSERT_EQ(
        engine.startProcessWafer(
            wafer_.id, destination_, recipe_
        ),
        StartResult::Accepted
    );

    // Wait for the equipment to return to Idle,
    // without calling waitForCompletion().
    const auto deadline =
        std::chrono::steady_clock::now() +
        std::chrono::seconds(5);

    while (engine.state() != EquipmentState::Idle &&
           std::chrono::steady_clock::now() < deadline)
    {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(1)
        );
    }

    ASSERT_EQ(engine.state(), EquipmentState::Idle);

    // A new run should safely replace the old worker.
    ASSERT_EQ(
        engine.startProcessWafer(
            wafer_.id, destination_, recipe_
        ),
        StartResult::Accepted
    );

    EXPECT_EQ(
        engine.waitForCompletion(),
        OperationResult::Completed
    );

    EXPECT_EQ(engine.state(), EquipmentState::Idle);
}

TEST_F(ProcessEngineTest,
    AcceptedRunCanLaterFailDuringExecution)
{
    MockRobot robot{true};
    MockProcessModule processModule{false};

    WaferManager mgr;
    mgr.addWafer(wafer_);

    ProcessEngine engine(
        mgr, robot, processModule, eventLog_
    );

    StartResult startResult = engine.startProcessWafer(
        wafer_.id,
        destination_,
        recipe_
    );

    EXPECT_EQ(startResult, StartResult::Accepted);

    OperationResult finalResult = engine.waitForCompletion();

    EXPECT_EQ(finalResult, OperationResult::Failed);
    EXPECT_EQ(engine.state(), EquipmentState::Faulted);
}

TEST_F(ProcessEngineTest, SuccessfulRunGeneratesRunCompletedEvent)
{
    MockRobot robot{true};
    MockProcessModule processModule{true};

    ProcessEngine engine(
        mgr_, robot, processModule, eventLog_
    );

    ASSERT_EQ(
        engine.processWafer(
            wafer_.id,
            destination_,
            recipe_
        ),
        OperationResult::Completed
    );

    const auto events = eventLog_.getEvents();

    const auto count = std::ranges::count_if(
        events,
        [](const EquipmentEvent& event)
        {
            return event.type == EventType::RunCompleted;
        }
    );

    EXPECT_EQ(count, 1);

    ASSERT_FALSE(events.empty());
    EXPECT_EQ(
        events.back().type,
        EventType::RunCompleted
    );

    EXPECT_EQ(engine.state(), EquipmentState::Idle);
}

TEST_F(ProcessEngineTest, FailedRunGeneratesRunFailedEvent)
{
    MockRobot robot{true};
    MockProcessModule processModule{false};

    ProcessEngine engine(mgr_, robot, processModule, eventLog_);

    ASSERT_EQ(
        engine.processWafer(wafer_.id, destination_, recipe_),
        OperationResult::Failed
    );

    const auto events = eventLog_.getEvents();

    EXPECT_EQ(
        std::ranges::count_if(events, [](const EquipmentEvent& event) {
            return event.type == EventType::RunFailed;
        }),
        1
    );

    EXPECT_EQ(events.back().type, EventType::RunFailed);
    EXPECT_EQ(engine.state(), EquipmentState::Faulted);
}

TEST_F(ProcessEngineTest, CancelledRunGeneratesRunCancelledEvent)
{
    MockRobot robot{true};
    BlockingMockProcessModule processModule;

    WaferManager mgr;
    mgr.addWafer(wafer_);

    ProcessEngine engine(mgr, robot, processModule, eventLog_);

    ASSERT_EQ(
        engine.startProcessWafer(wafer_.id, destination_, longRecipe_),
        StartResult::Accepted
    );

    processModule.waitUntilProcessingStarts();

    EXPECT_TRUE(engine.cancel().isCancelled);

    EXPECT_EQ(
        engine.waitForCompletion(),
        OperationResult::Cancelled
    );

    const auto events = eventLog_.getEvents();

    EXPECT_EQ(
        std::ranges::count_if(events, [](const EquipmentEvent& event) {
            return event.type == EventType::RunCancelled;
        }),
        1
    );

    EXPECT_EQ(events.back().type, EventType::RunCancelled);
    EXPECT_EQ(engine.state(), EquipmentState::Idle);
}

TEST_F(ProcessEngineTest, ExceptionGeneratesRunFailedEvent)
{
    MockRobot robot{true};
    ThrowingMockProcessModule processModule;

    WaferManager mgr;
    mgr.addWafer(wafer_);

    ProcessEngine engine(mgr, robot, processModule, eventLog_);

    ASSERT_EQ(
        engine.startProcessWafer(wafer_.id, destination_, recipe_),
        StartResult::Accepted
    );

    EXPECT_THROW(
        engine.waitForCompletion(),
        std::runtime_error
    );

    const auto events = eventLog_.getEvents();

    EXPECT_EQ(
        std::ranges::count_if(events, [](const EquipmentEvent& event) {
            return event.type == EventType::RunFailed;
        }),
        1
    );

    EXPECT_EQ(events.back().type, EventType::RunFailed);
    EXPECT_EQ(engine.state(), EquipmentState::Faulted);
}

TEST_F(ProcessEngineTest, AsyncRunGeneratesRunCompletedEvent)
{
    MockRobot robot{true};
    MockProcessModule processModule{true};

    WaferManager mgr;
    mgr.addWafer(wafer_);

    ProcessEngine engine(mgr, robot, processModule, eventLog_);

    ASSERT_EQ(
        engine.startProcessWafer(wafer_.id, destination_, recipe_),
        StartResult::Accepted
    );

    ASSERT_EQ(
        engine.waitForCompletion(),
        OperationResult::Completed
    );

    const auto events = eventLog_.getEvents();

    EXPECT_EQ(
        std::ranges::count_if(events, [](const EquipmentEvent& event) {
            return event.type == EventType::RunCompleted;
        }),
        1
    );

    EXPECT_EQ(events.back().type, EventType::RunCompleted);
}

TEST_F(ProcessEngineTest, CancelledProcessingRestoresOriginalWaferLocation)
{
    MockRobot robot{true};
    BlockingMockProcessModule processModule;

    WaferManager manager;
    const Location original{Station::LoadPort, 3};
    const Location destination{Station::ProcessModule, 1};

    ASSERT_EQ(
        manager.addWafer({"W1", original}),
        WaferRegistrationResult::Registered
    );

    ProcessEngine engine(
        manager, robot, processModule, eventLog_
    );

    ASSERT_EQ(
        engine.startProcessWafer("W1", destination, recipe_),
        StartResult::Accepted
    );

    processModule.waitUntilProcessingStarts();

    EXPECT_EQ(
        manager.getLocation("W1"),
        destination
    );

    EXPECT_TRUE(engine.cancel().isCancelled);

    const auto result = engine.waitForCompletion();

    EXPECT_EQ(result, OperationResult::Cancelled);
    EXPECT_EQ(engine.state(), EquipmentState::Idle);

// Successful cancellation recovery returns the wafer
// to its original LoadPort location.
EXPECT_EQ(manager.getLocation("W1"), original);

EXPECT_TRUE(
    manager.isLocationAvailable("W2", destination)
);

}

TEST_F(ProcessEngineTest, FailedProcessingPreservesWaferLocation)
{
    MockRobot robot{true};
    MockProcessModule processModule{false};

    WaferManager manager;

    const Location original{Station::LoadPort, 3};
    const Location destination{Station::ProcessModule, 1};

    ASSERT_EQ(
        manager.addWafer({"W1", original}),
        WaferRegistrationResult::Registered
    );

    ProcessEngine engine(
        manager, robot, processModule, eventLog_
    );

    const auto result =
        engine.processWafer("W1", destination, recipe_);

    EXPECT_EQ(result, OperationResult::Failed);
    EXPECT_EQ(engine.state(), EquipmentState::Faulted);

    // Processing failed after the wafer reached the module.
    EXPECT_EQ(manager.getLocation("W1"), destination);
}

TEST_F(ProcessEngineTest, RejectsNewRunWhenProcessModuleIsOccupied)
{
    MockRobot robot{true};
    MockProcessModule processModule{true};
    WaferManager manager;

    ASSERT_EQ(
        manager.addWafer({"W1", {Station::LoadPort, 1}}),
        WaferRegistrationResult::Registered
    );

    ASSERT_EQ(
        manager.addWafer({"W2", {Station::LoadPort, 2}}),
        WaferRegistrationResult::Registered
    );

    ASSERT_EQ(
        manager.updateLocation("W1", {Station::ProcessModule, 1}),
        LocationUpdateResult::Updated
    );

    ProcessEngine engine(manager, robot, processModule, eventLog_);

    EXPECT_EQ(
        engine.startProcessWafer(
            "W2",
            {Station::ProcessModule, 1},
            recipe_
        ),
        StartResult::Rejected
    );

    EXPECT_EQ(engine.state(), EquipmentState::Idle);

    EXPECT_EQ(
        manager.getLocation("W1"),
        (Location{Station::ProcessModule, 1})
    );

    EXPECT_EQ(
        manager.getLocation("W2"),
        (Location{Station::LoadPort, 2})
    );
}

TEST_F(ProcessEngineTest, RejectsRunWhenMockReportsDestinationOccupied)
{
    auto wafer = std::make_unique<Wafer>(wafer_);

    MockWaferManager manager(std::move(wafer));
    manager.setLocationAvailable(false);

    MockRobot robot{true};
    MockProcessModule processModule{true};

    ProcessEngine engine(
        manager, robot, processModule, eventLog_
    );

    EXPECT_EQ(
        engine.startProcessWafer(
            wafer_.id,
            destination_,
            recipe_
        ),
        StartResult::Rejected
    );

    EXPECT_EQ(engine.state(), EquipmentState::Idle);
}

TEST_F(ProcessEngineTest,
    RejectedOccupiedDestinationDoesNotEmitRunAccepted)
{
    MockRobot robot{true};
    MockProcessModule processModule{true};
    WaferManager manager;

    ASSERT_EQ(
        manager.addWafer({"W1", {Station::LoadPort, 1}}),
        WaferRegistrationResult::Registered
    );

    ASSERT_EQ(
        manager.addWafer({"W2", {Station::LoadPort, 2}}),
        WaferRegistrationResult::Registered
    );

    ASSERT_EQ(
        manager.updateLocation(
            "W1",
            {Station::ProcessModule, 1}
        ),
        LocationUpdateResult::Updated
    );

    ProcessEngine engine(
        manager, robot, processModule, eventLog_
    );

    // Capture events generated during admission only.
    const auto eventsBefore = eventLog_.getEvents().size();

    EXPECT_EQ(
        engine.startProcessWafer(
            "W2",
            {Station::ProcessModule, 1},
            recipe_
        ),
        StartResult::Rejected
    );

    const auto events = eventLog_.getEvents();

    auto forbiddenEventCount = std::ranges::count_if(
        events.begin() + eventsBefore,
        events.end(),
        [](const EquipmentEvent& event)
        {
            return event.type == EventType::RunAccepted ||
                   event.type == EventType::EquipmentMoving;
        }
    );

    EXPECT_EQ(forbiddenEventCount, 0);
    EXPECT_EQ(engine.state(), EquipmentState::Idle);

    EXPECT_EQ(
        manager.getLocation("W1"),
        (Location{Station::ProcessModule, 1})
    );

    EXPECT_EQ(
        manager.getLocation("W2"),
        (Location{Station::LoadPort, 2})
    );
}

TEST_F(ProcessEngineTest, CancelledProcessingReturnsWaferToLoadPort)
{
    // Arrange
    MockRobot robot{ true };
    BlockingMockProcessModule processModule;
    WaferManager mgr;

    const Location originalLocation{
        Station::LoadPort, 2
    };

    const Location processLocation{
        Station::ProcessModule, 1
    };

    ASSERT_EQ(
        mgr.addWafer(wafer_),
        WaferRegistrationResult::Registered
    );

    ProcessEngine engine(
        mgr, robot, processModule, eventLog_
    );

    // Act
    ASSERT_EQ(
        engine.startProcessWafer(
            wafer_.id,
            processLocation,
            longRecipe_
        ),
        StartResult::Accepted
    );

    processModule.waitUntilProcessingStarts();

    EXPECT_EQ(
        mgr.getLocation(wafer_.id),
        processLocation
    );

    const auto cancellation = engine.cancel();

    ASSERT_TRUE(cancellation.isCancelled);

    const auto result = engine.waitForCompletion();

    // Assert
    EXPECT_EQ(result, OperationResult::Cancelled);

    EXPECT_EQ(
        mgr.getLocation(wafer_.id),
        originalLocation
    );

    EXPECT_TRUE(
        mgr.isLocationAvailable("W2", processLocation)
    );

    EXPECT_EQ(engine.state(), EquipmentState::Idle);
}

TEST_F(ProcessEngineTest, FailedCancellationRecoveryLeavesEquipmentFaulted)
{
    MockRobot robot{ true };
    BlockingMockProcessModule processModule;
    WaferManager mgr;

    mgr.addWafer(wafer_);

    const Location processLocation{
        Station::ProcessModule, 1
    };

    ProcessEngine engine(
        mgr, robot, processModule, eventLog_
    );

    ASSERT_EQ(
        engine.startProcessWafer(
            wafer_.id,
            processLocation,
            longRecipe_
        ),
        StartResult::Accepted
    );

    processModule.waitUntilProcessingStarts();

    // First movement succeeded.
    // Make the return movement fail.
    robot.setMoveShouldSucceed(false);

    ASSERT_TRUE(engine.cancel().isCancelled);

    const auto result = engine.waitForCompletion();

    EXPECT_EQ(result, OperationResult::Failed);
    EXPECT_EQ(engine.state(), EquipmentState::Faulted);

    EXPECT_EQ(
        mgr.getLocation(wafer_.id),
        processLocation
    );

    EXPECT_FALSE(
        mgr.isLocationAvailable("W2", processLocation)
    );
}

TEST_F(ProcessEngineTest, FailedLocationUpdateDuringCancellationRecoveryFaultsEquipment)
{
    MockRobot robot{ true };
    BlockingMockProcessModule processModule;

    MockWaferManager mgr{
        std::make_unique<Wafer>(wafer_), true
    };

    // First update succeeds; return update fails.
    mgr.failOnLocationUpdate(2);

    const Location processLocation{
        Station::ProcessModule, 1
    };

    ProcessEngine engine(
        mgr, robot, processModule, eventLog_
    );

    ASSERT_EQ(
        engine.startProcessWafer(
            wafer_.id,
            processLocation,
            longRecipe_
        ),
        StartResult::Accepted
    );

    processModule.waitUntilProcessingStarts();

    ASSERT_EQ(mgr.locationUpdateCount(), 1);

    ASSERT_TRUE(engine.cancel().isCancelled);

    const auto result = engine.waitForCompletion();

    EXPECT_EQ(result, OperationResult::Failed);
    EXPECT_EQ(engine.state(), EquipmentState::Faulted);
    EXPECT_EQ(mgr.locationUpdateCount(), 2);
}

TEST_F(ProcessEngineTest, FailedManualWaferRecoveryFaultsEquipment)
{
    MockRobot robot{false};
    MockProcessModule processModule{true};

    WaferManager manager;

    const Location original{Station::LoadPort, 4};
    const Location stranded{Station::ProcessModule, 1};

    ASSERT_EQ(
        manager.addWafer({"W2", original}),
        WaferRegistrationResult::Registered
    );

    ASSERT_EQ(
        manager.updateLocation("W2", stranded),
        LocationUpdateResult::Updated
    );

    ProcessEngine engine(
        manager, robot, processModule, eventLog_
    );

    const auto result = engine.recoverWafer(
        "W2", original
    );

    EXPECT_EQ(result, WaferRecoveryResult::RobotFailed);
    EXPECT_EQ(engine.state(), EquipmentState::Faulted);

    // Failed movement must not change tracked location.
    EXPECT_EQ(manager.getLocation("W2"), stranded);

    // PM1 must remain occupied.
    EXPECT_FALSE(
        manager.isLocationAvailable(
            "W1", stranded
        )
    );
}


TEST_F(ProcessEngineTest, RecoveryPersistenceExceptionFaultsEquipment)
{
    const Location stranded{
        Station::ProcessModule, 1
    };

    const Location destination{
        Station::LoadPort, 4
    };

    auto wafer = std::make_unique<Wafer>(
        Wafer{"W2", stranded}
    );

    MockWaferManager manager(std::move(wafer));
    manager.setThrowOnLocationUpdate(true);

    MockRobot robot{true};
    MockProcessModule processModule{true};

    ProcessEngine engine(
        manager,
        robot,
        processModule,
        eventLog_
    );

    WaferRecoveryResult result = WaferRecoveryResult::Recovered;

    EXPECT_NO_THROW({
        result = engine.recoverWafer(
            "W2", destination
        );
    });

    EXPECT_EQ(result, WaferRecoveryResult::TrackingFailed);
    EXPECT_EQ(engine.state(), EquipmentState::Faulted);
    EXPECT_EQ(manager.locationUpdateCount(), 1);

    // The update threw, so the tracked location
    // must not be considered successfully updated.
    ASSERT_NE(manager.findWafer("W2"), nullptr);
    EXPECT_EQ(
        manager.findWafer("W2")->location,
        stranded
    );
}

TEST_F(ProcessEngineTest, RecoveryReportsMissingWafer)
{
    const auto result = engine_.recoverWafer(
        "DOES_NOT_EXIST",
        Location{Station::LoadPort, 4}
    );

    EXPECT_EQ(
        result,
        WaferRecoveryResult::WaferNotFound
    );

    EXPECT_EQ(
        engine_.state(),
        EquipmentState::Idle
    );
}

}
