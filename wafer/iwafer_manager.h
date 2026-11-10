#pragma once
#include "wafer/location_update_result.h"
#include "wafer/wafer_registration_result.h"
#include "domain/wafer.h"

class IWaferManager
{
public:
    virtual WaferRegistrationResult addWafer(const Wafer& wafer) = 0;

    virtual Wafer* findWafer(const std::string& waferId) = 0;

    virtual LocationUpdateResult updateLocation(const std::string& waferId, Location location) = 0;

    bool isLocationAvailable(const std::String& waferId, const Location& location) const;

    virtual ~IWaferManager() = default;
};
