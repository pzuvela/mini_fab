#pragma once

#include "domain/location.h"
#include "domain/wafer.h"

// C++, unlike C# has no interface keyword, we  define abstract (base) classes with pure virtuals instead

class IRobot
{
public:
    virtual void initialize() = 0;  // virtual enables runtime polymorphism.
                                    // 0 means that IRobot defines that this operation must exist, 
                                    // but doesn't provide the implementation here.
    virtual bool moveWafer(
        Wafer& wafer,
        const Location& destination
    ) = 0;
    virtual bool isReady() const = 0;

    virtual ~IRobot() = default;
};