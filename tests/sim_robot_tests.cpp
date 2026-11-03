#include <gtest/gtest.h>
#include <chrono>
#include <thread>

#include "domain/station.h"
#include "domain/location.h"
#include "domain/wafer.h"
#include "equipment/robot/sim_robot.h"

class SimRobotTest : public ::testing::Test
{
protected:
	Location source_{ Station::LoadPort, 2 };
	Location destination_{ Station::ProcessModule, 20 };

	Wafer wafer1_{"W001", source_};
	Wafer wafer2_{"W002", source_};

	SimRobot robot_{std::chrono::milliseconds{1}};
};

TEST_F(SimRobotTest, CanMoveAnotherWaferAfterSuccessfulMove)
{
	OperationResult firstResult = robot_.moveWafer(wafer1_, destination_, {});
	OperationResult secondResult = robot_.moveWafer(wafer2_, destination_, {});

	EXPECT_EQ(firstResult, OperationResult::Completed);
	EXPECT_EQ(secondResult, OperationResult::Completed);
}

TEST_F(SimRobotTest, CancelledMoveAllowsAnotherWaferToMove)
{
	SimRobot robot{ std::chrono::milliseconds{10} };

	OperationResult firstResult;


	std::jthread moveThread([&](std::stop_token stopToken)
	{
		firstResult = robot.moveWafer(wafer1_, destination_, stopToken);
	});

	while (!robot.carriedWaferId().has_value())
	{
		std::this_thread::yield();
	}

	EXPECT_EQ(robot.carriedWaferId().value(), wafer1_.id);

	moveThread.request_stop();
	moveThread.join();

	EXPECT_FALSE(robot.carriedWaferId().has_value());

	OperationResult secondResult = robot.moveWafer(wafer2_, destination_, {});

	EXPECT_EQ(firstResult, OperationResult::Cancelled);
	EXPECT_EQ(secondResult, OperationResult::Completed);

}
