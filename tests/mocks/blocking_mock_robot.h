#include <mutex>
#include <condition_variable>
#include <stop_token>

#include "equipment/operation_result.h"
#include "equipment/robot/irobot.h"

class BlockingMockRobot : public IRobot
{
private:
	bool ready_ = false;
	bool movementStarted_ = false;
	bool released_ = false;
	std::mutex mutex_;
	std::condition_variable_any condition_;

public:
	void initialize() override;
	bool isReady() const override;
	OperationResult moveWafer(
		Wafer& wafer,
		const Location& destination,
		std::stop_token stopToken
	) override;
	void waitUntilMovementStarts();
	void release();
};


