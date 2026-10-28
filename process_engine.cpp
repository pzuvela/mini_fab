#include <iostream>
#include "process_engine.h"
#include "iprocess_module.h"

ProcessEngine::ProcessEngine(IRobot& robot, IProcessModule& processModule)
	: robot_(robot), processModule_(processModule)
{
	std::cout << "Instantiated robot & process module\n";
}

bool ProcessEngine::run(
	Wafer& wafer,
	const Location& destination
)
{
	if (!robot_.isReady())
	{
		robot_.initialize();
	}
	bool success = robot_.moveWafer(wafer, destination);

	if (success)
	{
		wafer.location = destination;
	}

	return success;
}

bool ProcessEngine::process(
	const Wafer& wafer,
	const Recipe& recipe
) 
{
	if (!processModule_.isReady())
	{
		processModule_.initialize();
	}
	return processModule_.process(wafer, recipe);
}

bool ProcessEngine::processWafer(
	Wafer& wafer,
	const Location& processModuleLocation,
	const Recipe& recipe
)
{
	if (state_ != EquipmentState::Idle)
	{
		return false;
	}

	state_ = EquipmentState::Moving;

	bool isMovementSuccessful = run(wafer, processModuleLocation);

	if (!isMovementSuccessful) 
	{
		state_ = EquipmentState::Faulted;
		return false;
	}

	state_ = EquipmentState::Processing;

	bool isProcessSuccessful = process(wafer, recipe);

	if (isProcessSuccessful)
	{
		state_ = EquipmentState::Idle;
	}
	else
	{
		state_ = EquipmentState::Faulted;
	}

	return isProcessSuccessful;
}

EquipmentState ProcessEngine::state() const 
{
	return state_;
}

bool ProcessEngine::reset() 
{

	if (state_ != EquipmentState::Faulted)
	{
		return false;
	}

	state_ = EquipmentState::Idle;

	return true;
}
