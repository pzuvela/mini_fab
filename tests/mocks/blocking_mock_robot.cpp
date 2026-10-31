#include "blocking_mock_robot.h"

void BlockingMockRobot::initialize() 
{
	ready_ = true;
}

bool BlockingMockRobot::isReady() const 
{
	return ready_;
}

bool BlockingMockRobot::moveWafer(
	Wafer& wafer,
	const Location& location
)
{
	// unique_lock can do manual unlock/ lock, lock_guard has automatic lock/ unlock in/ out of scope
	// unique_lock is required by condition_variable, condition_variable::wait() needs to temporarily unlock the mutex while sleeping, wake up, lock mutex again
	std::unique_lock<std::mutex> lock(mutex_);

	movementStarted_ = true;

	condition_.notify_all();

	condition_.wait(lock, [this]()
	{
		return released_;
	)};

	return true;
}


void BlockingMockRobot::waitUntilMovementStarts()
{
	std::unique_lock<std::mutex> lock(mutex_);
	
	condition_.wait(lock, [this]()
	{
		return movementStarted_;
	});
}

void BlockingMockRobot::release()
{
	{
		std::lock_guard<std::mutex> lock(mutex_);
		released_ = true;
	}

	condition_.notify_all();
}
