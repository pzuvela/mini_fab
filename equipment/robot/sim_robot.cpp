#include "sim_robot.h"
#include <iostream>

void SimRobot::initialize()
{
    ready_ = true;
    std::cout << "Simulator robot initialized!\n";
}

OperationResult SimRobot::moveWafer(
    Wafer& wafer,
    const Location& destination,
    std::stop_token stopToken
)
{
    std::cout << "Wafer " << wafer.id << " moved from " << wafer.location.slot << " to " << destination.slot << "\n";
    return OperationResult::Completed;
}

bool SimRobot::isReady() const
{
    return ready_;
}
