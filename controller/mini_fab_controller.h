#pragma once

#include "equipment_snapshot.h"
#include "wafer/wafer_manager.h"
#include "equipment/robot/sim_robot.h"
#include "equipment/process_module/sim_process_module.h"
#include "engine/process_engine.h"

class MiniFabController
{
private:
	// declaration order is important, engine last, so that it gets destroyed first,
	// allowing its thread to finish before components it references are destroyed
	WaferManager manager_;
	SimRobot robot_;
	SimProcessModule processModule_;
	ProcessEngine engine_;

public:
	MiniFabController();

	EquipmentSnapshot status() const;
};
