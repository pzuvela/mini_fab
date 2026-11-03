#include <iostream>
#include <thread>
#include <optional>

#include "wafer/wafer_tracking_recovery_context.h"
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

bool ProcessEngine::completeMovement()
{
	std::lock_guard<std::mutex> lock(mutex_);

	switch (state_)
	{
		case EquipmentState::Moving:
		case EquipmentState::Stopping:
			state_ = EquipmentState::Idle;
			return true;

		default:
			return false;
	}
}

OperationResult ProcessEngine::returnWaferToOriginalLocation(
    Wafer& wafer,
    const Location& originalLocation,
    std::stop_token stopToken
)
{
	bool hasTransitioned = tryTransition(EquipmentState::Processing, EquipmentState::Moving);
	if (!hasTransitioned) return resultFromFailedTransition(stopToken);

	MovementStageResult returnResult = run(wafer, originalLocation, stopToken);

	switch (returnResult.movementResult)
	{
		case OperationResult::Completed:
			if (returnResult.locationUpdateResult.has_value() && returnResult.locationUpdateResult.value() == LocationUpdateResult::Updated) 
			{
				if (!completeMovement()) return OperationResult::Failed;
				return OperationResult::Completed;
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

}

OperationResult ProcessEngine::moveWaferToProcessModule(
	Wafer& wafer,
        const Location& processModuleLocation,
        const Location& originalLocation,
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
				return OperationResult::Completed;
			}
			else
			{
				if (result.locationUpdateResult != std::nullopt && result.locationUpdateResult.value() == LocationUpdateResult::WaferNotFound)
				{
					recoveryContext_ = WaferTrackingRecoveryContext{
						wafer.id,
						originalLocation,
						processModuleLocation
					};
				}

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
}

OperationResult ProcessEngine::processWaferAtModule(
        Wafer& wafer,
        const Recipe& recipe,
        const Location& originalLocation,
        std::stop_token stopToken
)
{
	OperationResult operationResult = process(wafer, recipe, stopToken);

	switch (operationResult)
	{
		case OperationResult::Completed:
			return returnWaferToOriginalLocation(
				wafer,
				originalLocation,
				stopToken
			);

		case OperationResult::Cancelled:
		{
			bool hasTransitioned = tryTransition(EquipmentState::Stopping, EquipmentState::Idle);

			if (!hasTransitioned) return resultFromFailedTransition(stopToken);

			return OperationResult::Cancelled;
		}

		case OperationResult::Failed:
			setFaulted(FaultSource::ProcessModule);
			return OperationResult::Failed;
	}

	return OperationResult::Failed;
}

OperationResult ProcessEngine::executeProcessWafer(
	Wafer& wafer,
	const Location& processModuleLocation,
	const Recipe& recipe,
	std::stop_token stopToken
)
{
	Location originalLocation = wafer.location;

	OperationResult movementResult = moveWaferToProcessModule(
        	wafer,
		processModuleLocation,
        	originalLocation,
        	stopToken
	);

	if (movementResult != OperationResult::Completed) return movementResult;

	return processWaferAtModule(
		wafer,
		recipe,
		originalLocation,
		stopToken
	);
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

std::optional<FaultSource> ProcessEngine::beginRecovery()
{
	std::lock_guard<std::mutex> lock(mutex_);

	if (state_ != EquipmentState::Faulted || !faultSource_.has_value()) return std::nullopt;

	FaultSource faultSource = faultSource_.value();
	state_ = EquipmentState::Recovering;

	return faultSource;
}

RecoveryResult ProcessEngine::recoverFaultSource(FaultSource faultSource)
{
	switch (faultSource)
	{
		case FaultSource::Robot:
			return robot_.recover();

		case FaultSource::ProcessModule:
			return processModule_.recover();

		case FaultSource::WaferTracking:
			return RecoveryResult::Failed;
	}

	return RecoveryResult::Failed;
}

void ProcessEngine::completeRecovery(RecoveryResult recoveryResult) 
{
	std::lock_guard<std::mutex> lock(mutex_);

	switch (recoveryResult)
	{
		case RecoveryResult::Recovered:
			faultSource_ = std::nullopt;
			state_ = EquipmentState::Idle;
			break;

		case RecoveryResult::Failed:
		case RecoveryResult::NotRequired:
			state_ = EquipmentState::Faulted;
			break;
	}
}

RecoveryResult ProcessEngine::reset()
{

	std::optional<FaultSource> faultSource = beginRecovery();

	if (!faultSource.has_value()) return RecoveryResult::NotRequired;

	RecoveryResult recoveryResult = recoverFaultSource(faultSource.value());

	completeRecovery(recoveryResult);

	return recoveryResult;
}

StateTransition ProcessEngine::beginCancellation()
{
	std::lock_guard<std::mutex> lock(mutex_);

	switch (state_)
	{
		case EquipmentState::Moving:
		case EquipmentState::Processing:
		{
			StateTransition transition
			{
				.previousState = state_,
				.newState = EquipmentState::Stopping,
				.isCancelled = true
			};

			state_ = EquipmentState::Stopping;

			return transition;
		}

		case EquipmentState::Idle:
		case EquipmentState::Stopping:
		case EquipmentState::Recovering:
		case EquipmentState::Faulted:
			return StateTransition{
				.previousState = state_,
				.newState = state_,
				.isCancelled = false
			};
	}

	return StateTransition{
		.previousState = state_,
		.newState = state_,
		.isCancelled = false
	};

}

StateTransition ProcessEngine::cancel() 
{
	StateTransition stateTransition = beginCancellation();

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
