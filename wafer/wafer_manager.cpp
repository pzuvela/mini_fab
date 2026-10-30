#include <unordered_map>
#include <memory>
#include <string>

#include "domain/wafer.h"
#include "wafer_manager.h"


    bool WaferManager::addWafer(const Wafer& wafer)
    {
        std::pair<
            std::unordered_map<std::string, std::unique_ptr<Wafer>>::iterator,
            bool
        > result= wafers_.emplace(  // can also use auto [iterator, succeeded]
            wafer.id,
            std::make_unique<Wafer>(wafer)
        );

        return result.second;
    }

    Wafer* WaferManager::findWafer(const std::string& waferId)
    {
        std::unordered_map<std::string, std::unique_ptr<Wafer>>::iterator iterator = wafers_.find(waferId);

        if (iterator == wafers_.end()) return nullptr;

        return iterator->second.get();
    }

