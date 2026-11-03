#include <iostream>
#include <chrono>
#include <condition_variable>
#include <mutex>

#include "sim_robot.h"


SimRobot::SimRobot(std::chrono::milliseconds moveDuration) 
	: moveDuration_(moveDuration)
{

}

void SimRobot::initialize()
{
	std::lock_guard<std::mutex> lock(mutex_);
	ready_ = true;
	std::cout << "Simulator robot initialized!\n";
}

OperationResult SimRobot::moveWafer(
	Wafer& wafer,
	const Location& destination,
	std::stop_token stopToken
)
{

	{
		std::lock_guard<std::mutex> lock(mutex_);

		if (!ready_) ready_ = true;

		if (carriedWaferId_.has_value()) return OperationResult::Failed;

		carriedWaferId_ = wafer.id;
	}

	std::unique_lock<std::mutex> lock(mutex_);
	conditionVariable_.wait_for(
		lock, 
		stopToken, 
		moveDuration_, 
		[] 
		{
			return false; 
		}
	);

	carriedWaferId_.reset();

	if (stopToken.stop_requested())
		return OperationResult::Cancelled;

	std::cout << "Wafer " << wafer.id << " moved from " << wafer.location.slot << " to " << destination.slot << "\n";

	return OperationResult::Completed;
}

RecoveryResult SimRobot::recover()
{
	std::lock_guard<std::mutex> lock(mutex_);

	return carriedWaferId_.has_value() ? RecoveryResult::Failed : RecoveryResult::Recovered;
}

bool SimRobot::isReady() const
{
	std::lock_guard<std::mutex> lock(mutex_);
	return ready_;
}

std::optional<std::string> SimRobot::carriedWaferId() const 
{
	std::lock_guard<std::mutex> lock(mutex_);
	return carriedWaferId_;
}
