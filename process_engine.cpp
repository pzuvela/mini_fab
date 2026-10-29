#include <iostream>
#include "process_engine.h"
#include "iprocess_module.h"
#include "iwafer_manager.h"

ProcessEngine::ProcessEngine(IWaferManager& mgr, IRobot& robot, IProcessModule& processModule)
	: mgr_(mgr), robot_(robot), processModule_(processModule)
{
	std::cout << "Instantiated wafer manager, robot & process module\n";
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
	const std::string& waferId,
	const Location& processModuleLocation,
	const Recipe& recipe
)
{
	if (state_ != EquipmentState::Idle)
	{
		return false;
	}

	Wafer* wafer = mgr_.findWafer(waferId);

	if (wafer == nullptr) 
	{
		return false;
	}

	state_ = EquipmentState::Moving;

	bool isMovementSuccessful = run(*wafer, processModuleLocation);

	if (!isMovementSuccessful) 
	{
		state_ = EquipmentState::Faulted;
		return false;
	}

	state_ = EquipmentState::Processing;

	bool isProcessSuccessful = process(*wafer, recipe);

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
