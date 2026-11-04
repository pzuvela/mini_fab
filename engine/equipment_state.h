#pragma once

#include <string_view>

enum class EquipmentState
{
    Idle,
    Moving,
    Processing,
    Stopping,
    Recovering,
    Faulted
};

constexpr std::string_view toString(EquipmentState state)
{
	switch (state)
	{
		case EquipmentState::Idle: return "Idle";
		case EquipmentState::Moving: return "Moving";
		case EquipmentState::Processing: return "Processing";
		case EquipmentState::Stopping: return "Stopping";
		case EquipmentState::Recovering: return "Recovering";
		case EquipmentState::Faulted: return "Faulted";
	}

	return "Unknown";
}
