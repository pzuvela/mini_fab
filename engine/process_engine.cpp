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

	std::stop_token stopToken;

	return processModule_.process(wafer, recipe, stopToken);
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

OperationResult ProcessEngine::executeProcessWafer(
	Wafer& wafer,
	const Location& processModuleLocation,
	const Recipe& recipe,
	std::stop_token stopToken = {}
)
{
	bool isMovementSuccessful = run(wafer, processModuleLocation);

	if (!isMovementSuccessful)
	{
		setState(EquipmentState::Faulted);
		return OperationResult::Failed;
	}

	bool hasTransitioned = tryTransition(EquipmentState::Moving, EquipmentState::Processing);

	if (!hasTransitioned) 

	{
		if (stopToken.stop_requested())
		{
			return OperationResult::Cancelled;
		}

		return OperationResult::Failed;
	}

	bool isProcessSuccessful = process(wafer, recipe);

	if (isProcessSuccessful)
	{
		setState(EquipmentState::Idle);
		return OperationResult::Completed;
	}
	else
	{
		setState(EquipmentState::Faulted);
		return OperationResult::Failed;
	}
}

OperationResult ProcessEngine::processWafer(
	const std::string& waferId,
	const Location& processModuleLocation,
	const Recipe& recipe
)
{
	Wafer* wafer = acceptWaferForProcessing(waferId);

	if (wafer == nullptr) return OperationResult::Failed;

	return executeProcessWafer(
		*wafer,
		processModuleLocation,
		recipe
	);
}

OperationResult ProcessEngine::startProcessWafer(
	const std::string& waferId,
	const Location& processModuleLocation,
	const Recipe& recipe
)
{
	Wafer* wafer = acceptWaferForProcessing(waferId);

	if (wafer == nullptr) return OperationResult::Failed;

	std::promise<OperationResult> promise;

	resultFuture_ = promise.get_future();

	worker_ = std::jthread(
		[this, wafer, processModuleLocation, recipe, promise = std::move(promise)](std::stop_token stopToken) mutable
		{
			try
			{
				OperationResult operationResult = this->executeProcessWafer(*wafer, processModuleLocation, recipe, stopToken);
				promise.set_value(operationResult);
			}
			catch (...)
			{
				setState(EquipmentState::Faulted);
				promise.set_exception(std::current_exception());
			}
		}
	);
	return OperationResult::Completed;
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

StateTransition ProcessEngine::cancel() 
{
	StateTransition stateTransition;

	{
		std::lock_guard<std::mutex> lock(mutex_);

		switch (state_) 
		{
			case EquipmentState::Moving:
			case EquipmentState::Processing:
				stateTransition = StateTransition
				{
					.previousState = state_,
					.newState = EquipmentState::Stopping,
					.isCancelled = true
				};
				state_ = EquipmentState::Stopping;
				break;
			case EquipmentState::Idle:
			case EquipmentState::Stopping:
			case EquipmentState::Faulted:
				stateTransition = StateTransition
				{
					.previousState = state_,
					.newState = state_,
					.isCancelled = false
				};
				break;
		}
	}

	if (stateTransition.isCancelled) 
	{
		worker_.request_stop();
	}

	return stateTransition;
}

OperationResult ProcessEngine::waitForCompletion()
{
	if (!resultFuture_.valid())
	{
		throw std::logic_error("No pending process result");
	} 
	return resultFuture_.get();
}

bool ProcessEngine::tryTransition(EquipmentState expectedState, EquipmentState newState) 
{
	std::lock_guard<std::mutex> lock(mutex_);
	if (state_ != expectedState) return false;
	state_ = newState;
	return true;
}
