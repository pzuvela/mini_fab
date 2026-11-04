#pragma once

#include <optional>
#include <string>

#include "domain/location.h"
#include "engine/equipment_state.h"

struct EquipmentSnapshot
{
	EquipmentState state;
	std::optional<Location> waferLocation;
	std::optional<std::string> robotWaferId;
	std::optional<std::string> processModuleWaferId;
};

