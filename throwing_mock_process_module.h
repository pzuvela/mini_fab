#pragma once

#include <stdexcept>
#include "iprocess_module.h"

class ThrowingMockProcessModule : public IProcessModule
{
private:
    bool isReady_ = false;

public:
    void initialize() override
    {
        isReady_ = true;
    }

    bool isReady() const override
    {
        return isReady_;
    }

    bool process(const Wafer& wafer, const Recipe& recipe) override
    {
        throw std::runtime_error("Process module failure");
    }
};