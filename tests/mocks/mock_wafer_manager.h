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
	bool locationAvailable_ = true;
	int locationUpdateCount_ = 0;
	int failOnLocationUpdate_ = -1;
	bool throwOnLocationUpdate_ = false;

public:
	MockWaferManager(std::unique_ptr<Wafer> waferToReturn, bool locationUpdateSucceeds = true);
	WaferRegistrationResult addWafer(const Wafer& wafer) override;
	Wafer* findWafer(const std::string& waferId) override;
	LocationUpdateResult updateLocation(const std::string& waferId, Location location) override;
	bool locationUpdateWasCalled() const;
	bool isLocationAvailable(
		const std::string& waferId,
		const Location& location
	) const override;
	void setLocationAvailable(bool available);
	void failOnLocationUpdate(int updateNumber);
	int locationUpdateCount() const;
	void setThrowOnLocationUpdate(bool enabled);
};

