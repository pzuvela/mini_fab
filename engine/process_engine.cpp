#include <iostream>
#include <thread>
#include <optional>

#include "process_engine.h"
#include "equipment/process_module/iprocess_module.h"
#include "wafer/iwafer_manager.h"

ProcessEngine::ProcessEngine(IWaferManager& mgr, IRobot& robot, IProcessModule& processModule)
	: mgr_(mgr), robot_(robot), processModule_(processModule)
{
	std::cout << "Instantiated wafer manager, robot & process module\n";
}

MovementStageResult ProcessEngine::run(
	Wafer& wafer,
	const Location& destination,
	std::stop_token stopToken
)
{
	if (!robot_.isReady())
	{
		robot_.initialize();
	}

	OperationResult movementResult = robot_.moveWafer(wafer, destination, stopToken);

	std::optional<LocationUpdateResult> locationUpdateResult = std::nullopt;

	if (movementResult == OperationResult::Completed)
	{
		locationUpdateResult = mgr_.updateLocation(wafer.id, destination);
	}

	return MovementStageResult{
		movementResult,
		locationUpdateResult
	};
}

OperationResult ProcessEngine::process(
	const Wafer& wafer,
	const Recipe& recipe,
	std::stop_token stopToken
)
{
	if (!processModule_.isReady())
	{
		processModule_.initialize();
	}

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


OperationResult ProcessEngine::resultFromFailedTransition(std::stop_token stopToken)
{
	return stopToken.stop_requested() ? OperationResult::Cancelled : OperationResult::Failed;
}


void ProcessEngine::setState(EquipmentState newState) 
{
	std::lock_guard<std::mutex> lock(mutex_);
	state_ = newState;
}

void ProcessEngine::setFaulted(FaultSource faultSource) 
{
	std::lock_guard<std::mutex> lock(mutex_);
	state_ = EquipmentState::Faulted;
	faultSource_ = faultSource;
}

OperationResult ProcessEngine::executeProcessWafer(
	Wafer& wafer,
	const Location& processModuleLocation,
	const Recipe& recipe,
	std::stop_token stopToken
)
{
	MovementStageResult result = run(wafer, processModuleLocation, stopToken);

	bool hasTransitioned;

	switch (result.movementResult)
	{
		case OperationResult::Completed:
			if (result.locationUpdateResult != std::nullopt && result.locationUpdateResult.value() == LocationUpdateResult::Updated) 
			{
				hasTransitioned = tryTransition(EquipmentState::Moving, EquipmentState::Processing);
				if (!hasTransitioned) return resultFromFailedTransition(stopToken);
				break;
			}
			else 
			{
				setFaulted(FaultSource::WaferTracking);
				return OperationResult::Failed;
			}

		case OperationResult::Cancelled:
			hasTransitioned = tryTransition(EquipmentState::Stopping, EquipmentState::Idle);
			if (!hasTransitioned) return resultFromFailedTransition(stopToken);
			return OperationResult::Cancelled;

		case OperationResult::Failed:
			setFaulted(FaultSource::Robot);
			return OperationResult::Failed;
	}

	OperationResult operationResult = process(wafer, recipe, stopToken);

	switch (operationResult)
	{
		case OperationResult::Completed:
		case OperationResult::Cancelled:
			setState(EquipmentState::Idle);
			break;
		case OperationResult::Failed:
			setFaulted(FaultSource::ProcessModule);
			break;
	}

	return operationResult;
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
				setFaulted(FaultSource::ProcessModule);
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

RecoveryResult ProcessEngine::reset() 
{

	FaultSource faultSource;

	{
		std::lock_guard<std::mutex> lock(mutex_);

		if (state_ != EquipmentState::Faulted || !faultSource_.has_value())
		{
			return RecoveryResult::NotRequired;
		}

		faultSource = faultSource_.value();
		state_ = EquipmentState::Recovering;
	}

	RecoveryResult recoveryResult;

	switch (faultSource)
	{
		case FaultSource::Robot:
			recoveryResult = robot_.recover();
			break;
		case FaultSource::ProcessModule:
			recoveryResult = processModule_.recover();
			break;
	}

	{
                std::lock_guard<std::mutex> lock(mutex_);

		switch (recoveryResult)
		{
			case RecoveryResult::Recovered:
				faultSource_ = {};
				state_ = EquipmentState::Idle;
				break;

			case RecoveryResult::Failed:
			case RecoveryResult::NotRequired:
				state_ = EquipmentState::Faulted;
				break;
		}

	}

	return recoveryResult;
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
			case EquipmentState::Recovering:
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
