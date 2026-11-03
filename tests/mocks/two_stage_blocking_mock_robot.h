#pragma once

#include <condition_variable>
#include <mutex>
#include <stop_token>

#include "equipment/robot/irobot.h"


class TwoStageBlockingMockRobot : public IRobot
{
private:
    bool ready_ = false;
    int movementCount_ = 0;
    bool returnMovementStarted_ = false;

    mutable std::mutex mutex_;
    std::condition_variable_any condition_;

public:
    void initialize() override;

    bool isReady() const override;

    OperationResult moveWafer(
        Wafer& wafer,
        const Location& destination,
        std::stop_token stopToken
    ) override;

    RecoveryResult recover() override;

    void waitUntilReturnMovementStarts();
};

