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
#include "wafer/iwafer_repository.h"


class WaferManager : public IWaferManager
{
private:
	std::unordered_map<std::string, std::unique_ptr<Wafer>> wafers_;
	mutable std::mutex mutex_;
	IWaferRepository* repository_ = nullptr;
	bool isLocationOccupied(const std::string& waferId, const Location& location) const;
	bool isWaferIdEmpty(const std::string& waferId) const;
	bool isSlotNegativeOrZero(const int slot) const;
	WaferRegistrationResult validateWafer(const Wafer& wafer) const;
	void insertWaferToMemory(const Wafer& wafer);
	static bool isValidLocation(const Location& location);

public:
	WaferManager() = default;
	explicit WaferManager(IWaferRepository& repository);
	WaferRegistrationResult addWafer(const Wafer& wafer) override;
	LocationUpdateResult updateLocation(const std::string& waferId, Location location) override;
	Wafer* findWafer(const std::string& waferId) override;
	std::optional<Location> getLocation(
		const std::string& waferId
	) const;
	std::vector<Wafer> getWafers() const;
};
