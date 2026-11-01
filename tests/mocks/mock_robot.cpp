#include "mock_robot.h"
#include <iostream>

MockRobot::MockRobot(bool moveShouldSucceed)
    : moveShouldSucceed_(moveShouldSucceed)
{
    std::cout << "[MOCK] instantiated!\n";
}

void MockRobot::initialize()
{
    ready_ = true;
    std::cout << "[MOCK] initialized!\n";
}

OperationResult MockRobot::moveWafer(
	Wafer& wafer,
	const Location& destination,
	std::stop_token stopToken
)
{
    if (moveShouldSucceed_)
    {
        std::cout << "[MOCK] move requested for wafer: " << wafer.id << " from slot " << wafer.location.slot << "  to " << destination.slot << "\n";
    }
    return moveShouldSucceed_ ? OperationResult::Completed : OperationResult::Failed;
}

bool MockRobot::isReady() const
{
    return ready_;
}

void MockRobot::setMoveShouldSucceed(bool moveShouldSucceed) 
{
    moveShouldSucceed_ = moveShouldSucceed;
}
