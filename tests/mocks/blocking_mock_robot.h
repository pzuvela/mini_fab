#include <mutex>
#include <condition_variable>

#include "equipment/robot/irobot.h"

class BlockingMockRobot : public IRobot
{
private:
	bool ready_ = false;
	bool movementStarted_ = false;
	bool released_ = false;
	std::mutex mutex_;
	std::condition_variable condition_;

public:
	void initialize() override;
	bool isReady() const override;
	bool moveWafer(
		Wafer& wafer,
		const Location& destination
	) override;
	void waitUntilMovementStarts();
	void release();
};


