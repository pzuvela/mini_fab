#pragma once

#include <vector>
#include "domain/wafer.h"

class IWaferRepository
{
public:
    virtual ~IWaferRepository() = default;

    virtual std::vector<Wafer> getAll() const = 0;
    virtual bool insert(const Wafer& wafer) = 0;
    virtual bool updateLocation(
        const std::string& waferId,
        const Location& location) = 0;
};

