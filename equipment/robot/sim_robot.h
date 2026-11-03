#pragma once

#include <optional>
#include <chrono>
#include <condition_variable>
#include <mutex>

#include "irobot.h"

class SimRobot : public IRobot
{
private:
	bool ready_ = false;
	std::optional<std::string> carriedWaferId_;
	std::chrono::milliseconds moveDuration_;
	std::condition_variable_any conditionVariable_;
	mutable std::mutex mutex_;

public:
	SimRobot(std::chrono::milliseconds  moveDuration);
	void initialize() override;
	OperationResult moveWafer(
		Wafer& wafer,
		const Location& destination,
		std::stop_token stopToken
	) override;
	RecoveryResult recover() override;
	bool isReady() const override;
	std::optional<std::string> carriedWaferId() const;
};
