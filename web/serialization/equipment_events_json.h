#pragma once

#include <string>
#include <vector>

#include "equipment/equipment_event.h"


namespace MiniFab::Web
{
	std::string toJson(const std::vector<EquipmentEvent>& events);
}

