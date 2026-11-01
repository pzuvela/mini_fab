#pragma once

#include <stop_token>
#include "equipment/operation_result.h"
#include "equipment/recovery_result.h"
#include "domain/location.h"
#include "domain/wafer.h"

// C++, unlike C# has no interface keyword, we  define abstract (base) classes with pure virtuals instead

class IRobot
{
public:
    virtual void initialize() = 0;  // virtual enables runtime polymorphism.
                                    // 0 means that IRobot defines that this operation must exist, 
                                    // but doesn't provide the implementation here.
    virtual OperationResult moveWafer(
        Wafer& wafer,
        const Location& destination,
	std::stop_token stopToken
    ) = 0;
    virtual bool isReady() const = 0;
    virtual RecoveryResult recover() = 0;
    virtual ~IRobot() = default;
};
