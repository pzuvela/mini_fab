#pragma once

#include <string>
#include "domain/location.h"

struct WaferTrackingRecoveryContext
{
    std::string waferId;
    Location originalLocation;
    Location destination;
};
