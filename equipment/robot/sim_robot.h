// sim_robot.h
#pragma once

#include "irobot.h"

class SimRobot : public IRobot
{
private:
    bool ready_ = false;

public:
    void initialize() override;
	OperationResult moveWafer(
		Wafer& wafer,
		const Location& destination,
		std::stop_token stopToken
	) override;
    bool isReady() const override;
};
