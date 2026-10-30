#include "equipment/robot/irobot.h"

class MockRobot : public IRobot
{
private:
	bool ready_ = false;
	bool moveShouldSucceed_;

public:
	MockRobot(bool moveShouldSucceed);
	void initialize() override;
	bool moveWafer(
		Wafer& wafer,
		const Location& destination
	) override;
	bool isReady() const override;
	void setMoveShouldSucceed(bool moveShouldSucceed);
};
