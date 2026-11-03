#include <vector>
#include <memory>
#include <iostream>
#include <chrono>

#include "wafer/iwafer_manager.h"
#include "wafer/wafer_manager.h"
#include "equipment/robot/irobot.h"
#include "equipment/robot/sim_robot.h"
#include "engine/process_engine.h"
#include "equipment/process_module/sim_process_module.h"

std::string toString(Station station)
{
    switch (station)
    {
        case Station::LoadPort:
            return "LoadPort";

        case Station::Aligner:
            return "Aligner";

        case Station::ProcessModule:
            return "ProcessModule";

	case Station::Output:
		return "Output";
    }

    return "Unknown";
}


std::string toString(EquipmentState state)
{
    switch (state)
    {
        case EquipmentState::Idle:
            return "Idle";
        case EquipmentState::Moving:
            return "Moving";
        case EquipmentState::Processing:
            return "Processing";
        case EquipmentState::Stopping:
            return "Stopping";
        case EquipmentState::Recovering:
            return "Recovering";
        case EquipmentState::Faulted:
            return "Faulted";
    }

    return "Unknown";
}

std::string toString(OperationResult result)
{
    switch (result)
    {
        case OperationResult::Completed:
            return "Completed";

        case OperationResult::Cancelled:
            return "Cancelled";

        case OperationResult::Failed:
            return "Failed";
    }

    return "Unknown";
}


int main()
{
	Wafer wafer{ "W1", Location{ Station::LoadPort, 3 } };

	Location processModuleLocation{ Station::ProcessModule, 1 };

	Recipe recipe{ .id = 1, .name = "Recipe1", .durationSeconds = 2.0 };

	SimRobot robot{ std::chrono::milliseconds{300} };

	SimProcessModule processModule;
	WaferManager mgr;

	mgr.addWafer(wafer);

	ProcessEngine engine{mgr, robot, processModule};

	engine.startProcessWafer(wafer.id, processModuleLocation, recipe);

	while (engine.state() != EquipmentState::Idle && engine.state() != EquipmentState::Faulted)
	{
	    Wafer* currentWafer = mgr.findWafer(wafer.id);

	    std::cout
	        << "State: " << toString(engine.state())
	        << " | Wafer station: "
	        << toString(currentWafer->location.station)
	        << " slot " << currentWafer->location.slot
	        << " | Robot: "
	        << (robot.carriedWaferId().has_value()
	                ? robot.carriedWaferId().value()
	                : "empty")
	        << " | PM: "
	        << (processModule.processingWaferId().has_value()
	                ? processModule.processingWaferId().value()
	                : "empty")
	        << '\n';

	    if (engine.state() == EquipmentState::Processing)
	    {
		std::this_thread::sleep_for(
        		std::chrono::milliseconds{200}
	    	);

	    	StateTransition transition = engine.cancel();

		std::cout
			<< "Cancel requested: "
			<< (transition.isCancelled ? "accepted" : "ignored")
			<< "\n";
	    }

	    std::this_thread::sleep_for(
        	std::chrono::milliseconds{100}
	    );
	}

	OperationResult result = engine.waitForCompletion();

	std::cout
		<< "Result: " << toString(result)
		<< "\n";

	Wafer* currentWafer = mgr.findWafer(wafer.id);

	std::cout
	    << "Final state: " << toString(engine.state())
	    << " | Wafer: " << toString(currentWafer->location.station)
	    << " slot " << currentWafer->location.slot
 	    << '\n';

	return 0;
};

