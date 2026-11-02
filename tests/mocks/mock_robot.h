#include <stop_token>
#include "equipment/operation_result.h"
#include "equipment/robot/irobot.h"


class MockRobot : public IRobot
{
private:
	bool ready_ = false;
	bool moveShouldSucceed_;
	bool recoveryShouldSucceed_;
	bool recoverWasCalled_ = false;

public:
	MockRobot(bool moveShouldSucceed, bool recoveryShouldSucceed = true);
	void initialize() override;
	OperationResult moveWafer(
		Wafer& wafer,
		const Location& destination,
		std::stop_token stopToken
	) override;
	RecoveryResult recover() override;
	bool isReady() const override;
	void setMoveShouldSucceed(bool moveShouldSucceed);
	void setRecoveryShouldSucceed(bool recoveryShouldSucceed);
	bool recoverWasCalled() const;
};
