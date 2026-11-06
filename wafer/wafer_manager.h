#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>
#include <optional>
#include <vector>

#include "domain/wafer.h"
#include "iwafer_manager.h"
#include "wafer_registration_result.h"


class WaferManager : public IWaferManager
{
private:
	std::unordered_map<std::string, std::unique_ptr<Wafer>> wafers_;
	mutable std::mutex mutex_;

	bool isLocationOccupied(const std::string& waferId, const Location& location) const;
	bool isWaferIdEmpty(const std::string& waferId) const;
	bool isSlotNegativeOrZero(const int slot) const;

public:
	WaferRegistrationResult addWafer(const Wafer& wafer) override;
	LocationUpdateResult updateLocation(const std::string& waferId, Location location) override;
	Wafer* findWafer(const std::string& waferId) override;
	std::optional<Location> getLocation(
		const std::string& waferId
	) const;
	std::vector<Wafer> getWafers() const;
};
