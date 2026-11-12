#include <memory>

#include "domain/wafer.h"
#include "mock_wafer_manager.h"

MockWaferManager::MockWaferManager(std::unique_ptr<Wafer> waferToReturn, bool locationUpdateSucceeds)
	: waferToReturn_(std::move(waferToReturn)), locationUpdateSucceeds_(locationUpdateSucceeds)
{
}

WaferRegistrationResult MockWaferManager::addWafer(const Wafer& wafer)
{
	waferToReturn_ = std::make_unique<Wafer>(wafer);
	return WaferRegistrationResult::Registered;
}

Wafer* MockWaferManager::findWafer(const std::string& waferId)
{
	if (waferToReturn_ == nullptr) return nullptr;
	if (waferToReturn_->id != waferId) return nullptr;
	return waferToReturn_.get();
}

LocationUpdateResult MockWaferManager::updateLocation(const std::string& waferId, Location location)
{
	locationUpdateWasCalled_ = true;
	++locationUpdateCount_;

	if (locationUpdateCount_ == failOnLocationUpdate_) return LocationUpdateResult::WaferNotFound;

	return locationUpdateSucceeds_ ? LocationUpdateResult::Updated : LocationUpdateResult::WaferNotFound;
}

bool MockWaferManager::locationUpdateWasCalled() const
{
	return locationUpdateWasCalled_;
}

bool MockWaferManager::isLocationAvailable(
    const std::string& waferId,
    const Location& location
) const
{
    return locationAvailable_;
}

void MockWaferManager::setLocationAvailable(bool available)
{
    locationAvailable_ = available;
}

void MockWaferManager::failOnLocationUpdate(int updateNumber)
{
	failOnLocationUpdate_ = updateNumber;
}

int MockWaferManager::locationUpdateCount() const
{
	return locationUpdateCount_;
}

