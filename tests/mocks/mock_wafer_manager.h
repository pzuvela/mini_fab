#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "domain/wafer.h"
#include "iwafer_manager.h"

class MockWaferManager : public IWaferManager
{
private:
    std::unique_ptr<Wafer> waferToReturn_;
public:
    MockWaferManager(std::unique_ptr<Wafer> waferToReturn);
    bool addWafer(const Wafer& wafer) override;
    Wafer* findWafer(const std::string& waferId) override;
};
