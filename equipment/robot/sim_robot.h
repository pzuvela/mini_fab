#pragma once

#include <optional>
#include "irobot.h"

class SimRobot : public IRobot
{
private:
    bool ready_ = false;
	std::optional<std::string> carriedWaferId_;

public:
    void initialize() override;
	OperationResult moveWafer(
		Wafer& wafer,
		const Location& destination,
		std::stop_token stopToken
	) override;
    RecoveryResult recover() override;
    bool isReady() const override;
};
