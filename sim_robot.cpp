#include "sim_robot.h"
#include <iostream>

void SimRobot::initialize()
{
    ready_ = true;
    std::cout << "Simulator robot initialized!\n";
}

bool SimRobot::moveWafer(
    Wafer& wafer,
    const Location& destination
)
{
    std::cout << "Wafer " << wafer.id << " moved from " << wafer.location.slot << " to " << destination.slot << "\n";
    return true;
}

bool SimRobot::isReady() const
{
    return ready_;
}