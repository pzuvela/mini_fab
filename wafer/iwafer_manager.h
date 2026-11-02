#pragma once
#include "wafer/location_update_result.h"
#include "domain/wafer.h"

class IWaferManager
{
public:
    virtual bool addWafer(const Wafer& wafer) = 0;

    virtual Wafer* findWafer(const std::string& waferId) = 0;

    virtual LocationUpdateResult updateLocation(const std::string& waferId, Location location) = 0;

    virtual ~IWaferManager() = default;
};
