#include <unordered_map>
#include <memory>
#include <string>
#include <stdexcept>

#include "domain/wafer.h"
#include "wafer_manager.h"

WaferManager::WaferManager(IWaferRepository& repository)
	: repository_(&repository)
{
	for (const auto& wafer : repository.getAll())
	{
		const auto result = validateWafer(wafer);

		if (result != WaferRegistrationResult::Registered) throw std::runtime_error("Invalid persisted wafer inventory");

		insertWaferToMemory(wafer);
	}
}

WaferRegistrationResult WaferManager::validateWafer(const Wafer& wafer) const
{
    if (wafer.id.empty()) return WaferRegistrationResult::InvalidId;

    if (!isValidLocation(wafer.location)) return WaferRegistrationResult::InvalidLocation;

    if (wafers_.contains(wafer.id)) return WaferRegistrationResult::DuplicateId;

    if (isLocationOccupied(wafer.id, wafer.location)) return WaferRegistrationResult::SlotOccupied;

    return WaferRegistrationResult::Registered;
}

void WaferManager::insertWaferToMemory(const Wafer& wafer)
{
    wafers_.emplace(
        wafer.id,
        std::make_unique<Wafer>(wafer)
    );
}

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

	if (wafer.location.station != Station::LoadPort) return WaferRegistrationResult::InvalidLocation;

	const auto result = validateWafer(wafer);

	if (result != WaferRegistrationResult::Registered) return result;

	if (repository_ && !repository_->insert(wafer)) throw std::runtime_error("Failed to persist wafer registration");

	insertWaferToMemory(wafer);

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

	if (!isValidLocation(location)) return LocationUpdateResult::InvalidLocation;

	if (isLocationOccupied(waferId, location)) return LocationUpdateResult::SlotOccupied;

	if (iterator->second->location == location) return LocationUpdateResult::Updated;

	if (repository_ && !repository_->updateLocation(waferId, location)) throw std::runtime_error("Failed to persist wafer location");

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

bool WaferManager::isValidLocation(const Location& location)
{
	if (location.slot < 1) return false;

	switch (location.station)
	{
		case Station::LoadPort:
		case Station::Output:
			return true;

		case Station::Aligner:
		case Station::ProcessModule:
			return location.slot == 1;
	}

	return false;
}
