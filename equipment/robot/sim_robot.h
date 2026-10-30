// sim_robot.h
#pragma once

#include "irobot.h"

class SimRobot : public IRobot
{
private:
    bool ready_ = false;

public:
    void initialize() override;
	bool moveWafer(
		Wafer& wafer,
		const Location& destination
	) override;
    bool isReady() const override;
};