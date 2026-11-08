#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "domain/wafer.h"
#include "wafer/iwafer_manager.h"
#include "wafer/wafer_registration_result.h"


class MockWaferManager : public IWaferManager
{
private:
    std::unique_ptr<Wafer> waferToReturn_;
    bool locationUpdateWasCalled_ = false;
    bool locationUpdateSucceeds_;
public:
    MockWaferManager(std::unique_ptr<Wafer> waferToReturn, bool locationUpdateSucceeds = true);
    WaferRegistrationResult addWafer(const Wafer& wafer) override;
    Wafer* findWafer(const std::string& waferId) override;
    LocationUpdateResult updateLocation(const std::string& waferId, Location location) override;
    bool locationUpdateWasCalled() const;
};
