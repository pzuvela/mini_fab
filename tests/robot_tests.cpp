#include <gtest/gtest.h>

#include "sim_robot.h"
#include "mocks/mock_robot.h"
#include "domain/wafer.h"
#include "domain/location.h"

namespace MiniFab::Tests
{
	class RobotTest : public ::testing::Test
	{
	protected:
		Wafer wafer{
			.id = "1",
			.location = Location{
				.station = Station::LoadPort,
				.slot = 2
			}
		};

		Location destination{
			.station = Station::Aligner,
			.slot = 7
		};
	};

	TEST_F(RobotTest, IsNotReadyBeforeInitialization)
	{
		SimRobot robot = SimRobot();
		EXPECT_FALSE(robot.isReady());
	}

	TEST_F(RobotTest, IsReadyAfterInitialization)
	{
		SimRobot robot = SimRobot();
		robot.initialize();
		EXPECT_TRUE(robot.isReady());
	}

	TEST_F(RobotTest, MoveSucceedsWhenReady)
	{
		SimRobot robot = SimRobot();
		robot.initialize();

		bool isSuccessful = robot.moveWafer(
			wafer,
			destination
		);

		EXPECT_TRUE(isSuccessful);
	}

	TEST_F(RobotTest, SuccessfulMockReturnsTrue)
	{
		MockRobot robot{true};
		robot.initialize();

		bool isSuccessful = robot.moveWafer(
			wafer,
			destination
		);

		EXPECT_TRUE(isSuccessful);
	}

	TEST_F(RobotTest, FailingMockReturnsFalse)
	{
		MockRobot robot{ false };
		robot.initialize();

		bool isSuccessful = robot.moveWafer(
			wafer,
			destination
		);

		EXPECT_FALSE(isSuccessful);
	}
}
