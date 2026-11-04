#include <chrono>

#include "mini_fab_controller.h"
#include "domain/wafer.h"
#include "domain/location.h"
#include "domain/station.h"


MiniFabController::MiniFabController()
	: robot_(std::chrono::milliseconds{300}), engine_(manager_, robot_, processModule_)
{
	manager_.addWafer(
		Wafer{
			.id = "W1",
			.location = Location{ Station::LoadPort, 3 }
		}
	);
}

EquipmentSnapshot MiniFabController::status() const
{
	return EquipmentSnapshot{
		.state = engine_.state(),
		.waferLocation = manager_.getLocation("W1"),
		.robotWaferId = robot_.carriedWaferId(),
		.processModuleWaferId = processModule_.processingWaferId()
	};
}

OperationResult MiniFabController::runWafer()
{
	Recipe recipe{
		.id = 1,
		.name = "Recipe1",
		.durationSeconds = 50
	};

	return engine_.startProcessWafer(
		"W1",
		Location{Station::ProcessModule, 1},
		recipe
	);

}
