#pragma once

#include "../enums/station.h"

struct Location
{
    Station station;
    int slot;
    
    bool operator==(const Location&) const = default;
};
