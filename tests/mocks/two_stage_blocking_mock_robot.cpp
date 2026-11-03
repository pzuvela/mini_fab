#include "two_stage_blocking_mock_robot.h"


void TwoStageBlockingMockRobot::initialize()
{
	std::lock_guard<std::mutex> lock(mutex_);
        ready_ = true;
}

bool TwoStageBlockingMockRobot::isReady() const
{
	std::lock_guard<std::mutex> lock(mutex_);
        return ready_;
}

void TwoStageBlockingMockRobot::waitUntilReturnMovementStarts()
{
	std::unique_lock<std::mutex> lock(mutex_);

	condition_.wait(lock, [this]()
	{
		return returnMovementStarted_;
	});
}

OperationResult TwoStageBlockingMockRobot::moveWafer(
	Wafer& wafer,
	const Location& location,
	std::stop_token stopToken
)
{
	std::unique_lock<std::mutex> lock(mutex_);

	++movementCount_;

	if (movementCount_ == 1) return OperationResult::Completed;

	returnMovementStarted_ = true;
	condition_.notify_all();

	condition_.wait(
		lock,
		stopToken,
		[]()
		{
			return false;
		}
	);

	return OperationResult::Completed;
}

RecoveryResult TwoStageBlockingMockRobot::recover()
{
	return RecoveryResult::Recovered;
}
