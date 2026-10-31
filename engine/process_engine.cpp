#include <iostream>
#include <thread>

#include "process_engine.h"
#include "equipment/process_module/iprocess_module.h"
#include "wafer/iwafer_manager.h"

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

Wafer* ProcessEngine::acceptWaferForProcessing(const std::string& waferId)
{
	std::lock_guard<std::mutex> lock(mutex_);

	if (state_ != EquipmentState::Idle) return nullptr;

	Wafer* wafer = mgr_.findWafer(waferId);

	if (wafer != nullptr) 
	{
		state_ = EquipmentState::Moving;
	}

	return wafer;
}

void ProcessEngine::setState(EquipmentState newState) 
{
	std::lock_guard<std::mutex> lock(mutex_);
	state_ = newState;
}

bool ProcessEngine::executeProcessWafer(
	Wafer& wafer,
	const Location& processModuleLocation,
	const Recipe& recipe
)
{
	bool isMovementSuccessful = run(wafer, processModuleLocation);

	if (!isMovementSuccessful)
	{
		setState(EquipmentState::Faulted);
		return false;
	}

	setState(EquipmentState::Processing);

	bool isProcessSuccessful = process(wafer, recipe);

	if (isProcessSuccessful)
	{
		setState(EquipmentState::Idle);
	}
	else
	{
		setState(EquipmentState::Faulted);
	}

	return isProcessSuccessful;
}

bool ProcessEngine::processWafer(
	const std::string& waferId,
	const Location& processModuleLocation,
	const Recipe& recipe
)
{
	Wafer* wafer = acceptWaferForProcessing(waferId);

	if (wafer == nullptr)
		return false;

	return executeProcessWafer(
		*wafer,
		processModuleLocation,
		recipe
	);
}

bool ProcessEngine::startProcessWafer(
	const std::string& waferId,
	const Location& processModuleLocation,
	const Recipe& recipe
) 
{
	Wafer* wafer = acceptWaferForProcessing(waferId);

	if (wafer == nullptr) return false;

	std::promise<bool> promise;

	resultFuture_ = promise.get_future();


	worker_ = std::jthread(
		[this, wafer, processModuleLocation, recipe, promise = std::move(promise)]() mutable
		{
			try
			{
				bool isProcessSuccessful = this->executeProcessWafer(*wafer, processModuleLocation, recipe);
				promise.set_value(isProcessSuccessful);
			}
			catch (...)
			{
				setState(EquipmentState::Faulted);
				promise.set_exception(std::current_exception());
			}
		}
	);
	return true;
}

EquipmentState ProcessEngine::state() const 
{
	std::lock_guard<std::mutex> lock(mutex_);
	return state_;
}

bool ProcessEngine::reset() 
{
	std::lock_guard<std::mutex> lock(mutex_);

	if (state_ != EquipmentState::Faulted)
	{
		return false;
	}

	state_ = EquipmentState::Idle;

	return true;
}

bool ProcessEngine::waitForCompletion()
{	
	if (!resultFuture_.valid())
	{
		throw std::logic_error("No pending process result");
	} 
	return resultFuture_.get();
}
