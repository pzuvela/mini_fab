#pragma once
#include <string>
#include "location.h"

struct Wafer
{
    std::string id;
    Location location;

    bool operator==(const Wafer&) const = default;
};
