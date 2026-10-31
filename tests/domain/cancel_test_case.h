#include "engine/equipment_state.h"
#include "engine/state_transition.h"


struct CancelTestCase 
{
	EquipmentState initialState;
	StateTransition expected;
}
