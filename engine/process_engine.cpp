#include <iostream>
#include <thread>
#include <optional>

#include "wafer/wafer_tracking_recovery_context.h"
#include "process_engine.h"
#include "equipment/process_module/iprocess_module.h"
#include "wafer/iwafer_manager.h"

ProcessEngine::ProcessEngine(IWaferManager& mgr, IRobot& robot, IProcessModule& processModule, EventLog& eventLog)
	: mgr_(mgr), robot_(robot), processModule_(processModule), eventLog_(eventLog)
{
	std::cout << "Instantiated wafer manager, robot, process module & event log\n";
}

MovementStageResult ProcessEngine::executeMoveWafer(
	Wafer& wafer,
	const Location& destination,
	std::stop_token stopToken
)
{
	if (!robot_.isReady())
	{
		eventLog_.record(
			EventType::RobotInitializing,
			"Initializing Robot"
		);
		robot_.initialize();
		eventLog_.record(
			EventType::RobotReady,
			"Robot initialized"
		);
	}

	eventLog_.record(
		EventType::EquipmentMoving,
		"Moving wafer " + wafer.id
	);

	OperationResult movementResult = robot_.moveWafer(wafer, destination, stopToken);

	std::optional<LocationUpdateResult> locationUpdateResult = std::nullopt;

	if (movementResult == OperationResult::Completed)
	{
		locationUpdateResult = mgr_.updateLocation(wafer.id, destination);

		if (locationUpdateResult == LocationUpdateResult::Updated)
		{
			eventLog_.record(
				EventType::WaferLocationUpdated,
				"Location updated for wafer " + wafer.id + " to " + std::string(toString(destination.station)) + " / " + std::to_string(destination.slot)
			);
		}
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
		eventLog_.record(
			EventType::ProcessModuleInitializing,
			"Initializing process module"
		);
		processModule_.initialize();
		eventLog_.record(
			EventType::ProcessModuleReady,
			"Process module initialized"
		);
	}

	eventLog_.record(
		EventType::ProcessStarted,
		"Processing wafer " + wafer.id + " with recipe " + recipe.name
	);

	OperationResult result = processModule_.process(wafer, recipe, stopToken);

	switch (result)
	{
		case OperationResult::Completed:
			eventLog_.record(
				EventType::ProcessCompleted,
				"Processing completed for wafer " + wafer.id
			);
			break;
		case OperationResult::Cancelled:
			eventLog_.record(
				EventType::ProcessCancelled,
				"Processing cancelled for wafer " + wafer.id
			);
			break;
		case OperationResult::Failed:
			eventLog_.record(
				EventType::ProcessFailed,
				"Processing failed for wafer " + wafer.id
			);
			break;
	}

	return result;
}

Wafer* ProcessEngine::acceptWaferForProcessing(const std::string& waferId, const Location& destination, std::uint64_t& runId)
{
	std::lock_guard<std::mutex> lock(mutex_);

	if (state_ != EquipmentState::Idle) return nullptr;

	Wafer* wafer = mgr_.findWafer(waferId);

	if (wafer == nullptr) return nullptr;

	if (!mgr_.isLocationAvailable(waferId, destination)) return nullptr;

	state_ = EquipmentState::Moving;

	runId = eventLog_.record(
		EventType::RunAccepted,
		"Wafer " + waferId + " accepted for processing"
	);

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

	MovementStageResult returnResult = executeMoveWafer(wafer, originalLocation, stopToken);

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
	MovementStageResult result = executeMoveWafer(wafer, processModuleLocation, stopToken);

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
			const std::stop_token recoveryToken{};

			const auto movement = executeMoveWafer(wafer, originalLocation, recoveryToken);

			if (movement.movementResult != OperationResult::Completed || movement.locationUpdateResult != LocationUpdateResult::Updated)
			{
				setFaulted(FaultSource::Robot);
				return OperationResult::Failed;
			}

			bool hasTransitioned = tryTransition(EquipmentState::Stopping, EquipmentState::Idle);

			if (!hasTransitioned)
			{
				setFaulted(FaultSource::Robot);
				return OperationResult::Failed;
			}

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
	std::uint64_t& runId,
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

	OperationResult result = movementResult;

	if (movementResult == OperationResult::Completed)
	{
		result = processWaferAtModule(
			wafer,
			recipe,
			originalLocation,
			stopToken
		);
	}

	recordRunOutcome(wafer.id, result, runId);

	return result;
}

OperationResult ProcessEngine::processWafer(
	const std::string& waferId,
	const Location& processModuleLocation,
	const Recipe& recipe
)
{
	std::uint64_t runId = 0;

	Wafer* wafer = acceptWaferForProcessing(waferId, processModuleLocation, runId);

	if (wafer == nullptr) return OperationResult::Failed;

	return executeProcessWafer(
		*wafer,
		processModuleLocation,
		recipe,
		runId
	);
}

StartResult ProcessEngine::startProcessWafer(
	const std::string& waferId,
	const Location& processModuleLocation,
	const Recipe& recipe,
	std::uint64_t* acceptedRunId
)
{
	std::lock_guard<std::mutex> lifecycleLock(lifecycleMutex_);

	std::uint64_t runId = 0;

	Wafer* wafer = acceptWaferForProcessing(waferId, processModuleLocation, runId);

	if (wafer == nullptr) return StartResult::Rejected;

	if (acceptedRunId) *acceptedRunId = runId;

	if (worker_.joinable()) worker_.join();

	std::promise<OperationResult> promise;

	std::future<OperationResult> future = promise.get_future();

	try
	{
		std::jthread newWorker(
			[this, wafer, processModuleLocation, recipe, runId, promise = std::move(promise)](std::stop_token stopToken) mutable
			{
				try
				{
					OperationResult operationResult = this->executeProcessWafer(*wafer, processModuleLocation, recipe, runId, stopToken);
					promise.set_value(operationResult);
				}
				catch (...)
				{
					setFaulted(FaultSource::ProcessModule);
					recordRunOutcome(wafer->id, OperationResult::Failed, runId);
					promise.set_exception(std::current_exception());
				}
			}
		);

		worker_ = std::move(newWorker);
		resultFuture_ = std::move(future);
	}
	catch (...)
	{
		setFaulted(FaultSource::ProcessModule);
		throw;
	}
	return StartResult::Accepted;
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
	std::lock_guard<std::mutex> lifecycleLock(lifecycleMutex_);

	StateTransition stateTransition = beginCancellation();

	if (stateTransition.isCancelled)
	{
		worker_.request_stop();
	}

	return stateTransition;
}

OperationResult ProcessEngine::waitForCompletion()
{

	std::future<OperationResult> future;

	{
		std::lock_guard<std::mutex> lifecycleLock(lifecycleMutex_);
		if (!resultFuture_.valid())
		{
			throw std::logic_error("No pending process result");
		}

		future = std::move(resultFuture_);
	}


	return future.get();
}

bool ProcessEngine::tryTransition(EquipmentState expectedState, EquipmentState newState) 
{
	std::lock_guard<std::mutex> lock(mutex_);
	if (state_ != expectedState) return false;
	state_ = newState;
	return true;
}

void ProcessEngine::recordRunOutcome(
	const std::string& waferId,
	OperationResult result,
	std::uint64_t runId
)
{
    switch (result)
    {
        case OperationResult::Completed:
            eventLog_.record(
                EventType::RunCompleted,
                "Run completed for wafer " + waferId,
		runId
            );
            break;

        case OperationResult::Cancelled:
            eventLog_.record(
                EventType::RunCancelled,
                "Run cancelled for wafer " + waferId,
		runId
            );
            break;

        case OperationResult::Failed:
            eventLog_.record(
                EventType::RunFailed,
                "Run failed for wafer " + waferId,
		runId
            );
            break;
    }
}

RecoveryResult ProcessEngine::recoverWafer(
	const std::string& waferId,
	const Location& destination
)
{
	std::lock_guard<std::mutex> lifecycleLock(lifecycleMutex_);

	if (!tryTransition(EquipmentState::Idle, EquipmentState::Recovering)) return RecoveryResult::Failed;

	const auto reject = [this]()
	{
		setState(EquipmentState::Idle);
		return RecoveryResult::Failed;
	};

	Wafer* wafer = mgr_.findWafer(waferId);

	if (!wafer) return reject();

	if (wafer->location != Location{Station::ProcessModule, 1}) return reject();

	if (destination.station != Station::LoadPort || !mgr_.isLocationAvailable(waferId, destination)) return reject();

	const auto movement = executeMoveWafer(
		*wafer,
		destination,
		std::stop_token{}
	);

	if (movement.movementResult != OperationResult::Completed)
	{
		setFaulted(FaultSource::Robot);
		return RecoveryResult::Failed;
	}

	if (movement.movementResult != OperationResult::Updated)
	{
		setFaulted(FaultSource::WaferTracking);
		return RecoveryResult::Failed;
	}

	eventLog_.record(
		EventType::WaferLocationUpdated,
		"Recovered wafer " + waferId + 
		" to LoadPort / Slot " +
		std::to_string(destination.slot)
	);

	setState(EquipmentState::Idle);

	return RecoveryResult::Recovered;
}
