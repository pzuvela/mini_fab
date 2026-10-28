#include <gtest/gtest.h>

#include "process_engine.h"
#include "sim_process_module.h"
#include "mock_process_module.h"
#include "mock_robot.h"
#include "domain/wafer.h"
#include "domain/location.h"
#include "enums/equipment_state.h"

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

        SimProcessModule processModule_;
    };

    TEST_F(ProcessEngineTest, UpdatesWaferLocationWhenRobotMoveSucceeds)
    {
        MockRobot robot{ true };
        ProcessEngine engine(robot, processModule_);

        engine.run(wafer_, destination_);

        EXPECT_EQ(wafer_.location, destination_);
    }

    TEST_F(ProcessEngineTest, KeepsWaferLocationWhenRobotMoveFails)
    {
        MockRobot robot{ false };
        ProcessEngine engine(robot, processModule_);
        Location originalLocation = wafer_.location;

        engine.run(wafer_, destination_);

        EXPECT_EQ(wafer_.location, originalLocation);
    }

    TEST_F(ProcessEngineTest, ProcessReturnsTrueWhenProcessModuleSucceeds)
    {
        MockRobot robot{ true };
        MockProcessModule processModule{ true };
        ProcessEngine engine(robot, processModule);

        bool isSuccessful = engine.process(wafer_, recipe_);

        EXPECT_TRUE(isSuccessful);
    }

    TEST_F(ProcessEngineTest, ProcessReturnsFalseWhenProcessModuleFails)
    {
        MockRobot robot{ true };
        MockProcessModule processModule{ false };
        ProcessEngine engine(robot, processModule);

        bool isSuccessful = engine.process(wafer_, recipe_);

        EXPECT_FALSE(isSuccessful);
    }

    TEST_F(ProcessEngineTest, DoesNotProcessWaferWhenMovementFails)
    {
        MockRobot robot{ false };
        MockProcessModule processModule{ true };
        ProcessEngine engine(robot, processModule);

        bool isProcessed = engine.processWafer(wafer_, destination_, recipe_);

        EXPECT_FALSE(isProcessed);
        EXPECT_FALSE(processModule.processWasCalled());
        EXPECT_EQ(engine.state(), EquipmentState::Faulted);
    }

    TEST_F(ProcessEngineTest, ProcessesWaferWhenMovementSucceeds)
    {
        MockRobot robot{ true };
        MockProcessModule processModule{ true };
        ProcessEngine engine(robot, processModule);

        bool isProcessed = engine.processWafer(wafer_, destination_, recipe_);

        EXPECT_TRUE(isProcessed);
        EXPECT_TRUE(processModule.processWasCalled());
    }

    TEST_F(ProcessEngineTest, ReturnsFalseWhenProcessingFails)
    {
        MockRobot robot{ true };
        MockProcessModule processModule{ false };
        ProcessEngine engine(robot, processModule);

        bool isProcessed = engine.processWafer(wafer_, destination_, recipe_);

        EXPECT_FALSE(isProcessed);
        EXPECT_TRUE(processModule.processWasCalled());
        EXPECT_EQ(destination_, wafer_.location);
    }

    TEST_F(ProcessEngineTest, IsIdleWhenCreated)
    {
        MockRobot robot{ true };
        MockProcessModule processModule{ true };
        ProcessEngine engine(robot, processModule);
        
        EXPECT_EQ(engine.state(), EquipmentState::Idle);
    }

    TEST_F(ProcessEngineTest, DoesNotProcessWaferWhenEngineIsFaulted)
    {
        MockRobot robot{ false };
        MockProcessModule processModule{ true };
        ProcessEngine engine(robot, processModule);

        engine.processWafer(wafer_, destination_, recipe_);
        robot.setMoveShouldSucceed(true);

        bool isProcessed = engine.processWafer(wafer_, destination_, recipe_);

        EXPECT_FALSE(isProcessed);
        EXPECT_EQ(engine.state(), EquipmentState::Faulted);
        EXPECT_FALSE(processModule.processWasCalled());
    }

    TEST_F(ProcessEngineTest, ResetReturnsEngineToIdleAfterFault) 
    {
        MockRobot robot{ false };
        MockProcessModule processModule{ true };
        ProcessEngine engine(robot, processModule);

        engine.processWafer(wafer_, destination_, recipe_);

        bool isResetSuccessful = engine.reset();

        EXPECT_TRUE(isResetSuccessful);
        EXPECT_EQ(engine.state(), EquipmentState::Idle);
    }

    TEST_F(ProcessEngineTest, ResetFailsWhenEngineIsNotFaulted) 
    {
        MockRobot robot{ false };
        MockProcessModule processModule{ true };
        ProcessEngine engine(robot, processModule);

        bool isResetSuccessful = engine.reset();

        EXPECT_FALSE(isResetSuccessful);
        EXPECT_EQ(engine.state(), EquipmentState::Idle);
    }

    TEST_F(ProcessEngineTest, ProcessesWaferSuccessfullyAfterResettingFromFault) 
    {
        MockRobot robot{ false };
        MockProcessModule processModule{ true };
        ProcessEngine engine(robot, processModule);

        engine.processWafer(wafer_, destination_, recipe_);
        
        engine.reset();
        
        robot.setMoveShouldSucceed(true);

        bool isProcessed = engine.processWafer(wafer_, destination_, recipe_);

        EXPECT_TRUE(isProcessed);
        EXPECT_EQ(engine.state(), EquipmentState::Idle);
    }
}
