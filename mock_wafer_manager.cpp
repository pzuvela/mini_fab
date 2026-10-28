#include <unordered_map>
#include <memory>
#include <string>

#include "domain/wafer.h"
#include "mock_wafer_manager.h"

MockWaferManager::MockWaferManager(std::unique_ptr<Wafer> waferToReturn)
	: waferToReturn_(waferToReturn) 
{
}

    Wafer* WaferManager::findWafer(const std::string& waferId)
    {
	if (waferToReturn_.id != waferId) return nullptr;
	return waferToReturn_.get();
    }

