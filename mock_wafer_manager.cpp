#include <memory>

#include "domain/wafer.h"
#include "mock_wafer_manager.h"

MockWaferManager::MockWaferManager(std::unique_ptr<Wafer> waferToReturn)
	: waferToReturn_(std::move(waferToReturn))
{
}

bool MockWaferManager::addWafer(const Wafer& wafer)
{
	waferToReturn_ = std::make_unique<Wafer>(wafer);
	return true;
}

Wafer* MockWaferManager::findWafer(const std::string& waferId)
{
	if (waferToReturn_ == nullptr) return nullptr;
	if (waferToReturn_->id != waferId) return nullptr;
	return waferToReturn_.get();
}