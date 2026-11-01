#include <stop_token>
#include "equipment/operation_result.h"
#include "equipment/robot/irobot.h"


class MockRobot : public IRobot
{
private:
	bool ready_ = false;
	bool moveShouldSucceed_;

public:
	MockRobot(bool moveShouldSucceed);
	void initialize() override;
	OperationResult moveWafer(
		Wafer& wafer,
		const Location& destination,
		std::stop_token stopToken
	) override;
	RecoveryResult recover() override;
	bool isReady() const override;
	void setMoveShouldSucceed(bool moveShouldSucceed);
};
