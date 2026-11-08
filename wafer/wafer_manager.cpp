#include <unordered_map>
#include <memory>
#include <string>

#include "domain/wafer.h"
#include "wafer_manager.h"

bool WaferManager::isLocationOccupied(const std::string& waferId, const Location& location) const
{
	for (const auto& [existingWaferId, existingWafer] : wafers_)
        {
		if (waferId == existingWaferId) continue;
                if (location == existingWafer->location) return true;
        }

	return false;
}

bool WaferManager::isWaferIdEmpty(const std::string& waferId) const
{
	return waferId.empty();
}

bool WaferManager::isSlotNegativeOrZero(const int slot) const
{
	return slot < 1;
}


    WaferRegistrationResult WaferManager::addWafer(const Wafer& wafer)
    {
	std::lock_guard<std::mutex> lock(mutex_);

	if (isWaferIdEmpty(wafer.id)) return WaferRegistrationResult::InvalidId;

	if (isSlotNegativeOrZero(wafer.location.slot)) return WaferRegistrationResult::InvalidLocation;

	if (wafers_.contains(wafer.id)) return WaferRegistrationResult::DuplicateId;

	if (wafer.location.station != Station::LoadPort) return WaferRegistrationResult::InvalidLocation;

	if (isLocationOccupied(wafer.id, wafer.location)) return WaferRegistrationResult::SlotOccupied;

        wafers_.emplace(  // can also use auto [iterator, succeeded]
            wafer.id,
            std::make_unique<Wafer>(wafer)
        );

        return WaferRegistrationResult::Registered;
    }

    Wafer* WaferManager::findWafer(const std::string& waferId)
    {
	std::lock_guard<std::mutex> lock(mutex_);

        std::unordered_map<std::string, std::unique_ptr<Wafer>>::iterator iterator = wafers_.find(waferId);

        if (iterator == wafers_.end()) return nullptr;

        return iterator->second.get();
    }


LocationUpdateResult WaferManager::updateLocation(const std::string& waferId, Location location)
{
	std::lock_guard<std::mutex> lock(mutex_);

	auto iterator = wafers_.find(waferId);

	if (iterator == wafers_.end()) return LocationUpdateResult::WaferNotFound;

	iterator->second->location = location;

	return LocationUpdateResult::Updated;
}

std::optional<Location> WaferManager::getLocation(
	const std::string& waferId
) const
{
	std::lock_guard<std::mutex> lock(mutex_);

	auto iterator = wafers_.find(waferId);

	if (iterator == wafers_.end()) return std::nullopt;

	return iterator->second->location;
}

std::vector<Wafer> WaferManager::getWafers() const
{
	std::lock_guard<std::mutex> lock(mutex_);

	std::vector<Wafer> wafers;
	wafers.reserve(wafers_.size());

	for (const auto& [waferId, wafer] : wafers_)
	{
		wafers.emplace_back(*wafer);
	}

	return wafers;
}
