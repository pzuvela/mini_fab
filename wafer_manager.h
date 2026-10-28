#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "domain/wafer.h"
#include "iwafer_manager.h"

class WaferManager : public IWaferManager
{
private:
    std::unordered_map<std::string, std::unique_ptr<Wafer>> wafers_;

public:
    bool addWafer(const Wafer& wafer) override;
    Wafer* findWafer(const std::string& waferId) override;
};
