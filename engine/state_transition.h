#include "equipment_state.h"


struct StateTransition 
{
	EquipmentState previousState;
	EquipmentState newState;
	bool isCancelled;

	bool operator==(const StateTransition&) const = default;
};
