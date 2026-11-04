#pragma once

#include <string>
#include "controller/equipment_snapshot.h"

namespace MiniFab::Web
{
    std::string toJson(const EquipmentSnapshot& snapshot);
}

