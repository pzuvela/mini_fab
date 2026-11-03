#include <chrono>
#include <stdexcept>
#include <iostream>

#include "sim_process_module.h"

OperationResult SimProcessModule::process(const Wafer& wafer, const Recipe& recipe, std::stop_token stopToken)
{
	if (!isReady()) 
	{
		throw std::logic_error(
			"SimProcessModule must be initialized before process can be called."
		);
	}

	{
		std::unique_lock<std::mutex> lock(mutex_);

		if (processingWaferId_.has_value()) return OperationResult::Failed;

		processingWaferId_ = wafer.id;
	}

	std::unique_lock<std::mutex> lock(mutex_);

	conditionVariable_.wait_for(
		lock,
		stopToken,
		std::chrono::duration<double>{recipe.durationSeconds},
		[]
		{
			return false;
		}
	);

	processingWaferId_.reset();

	if (stopToken.stop_requested()) return OperationResult::Cancelled;

	std::cout << "Processing wafer " << wafer.id << " using " << recipe.name << " for " << recipe.durationSeconds << " seconds \n";

	return OperationResult::Completed;
}

RecoveryResult SimProcessModule::recover()
{
	std::lock_guard<std::mutex> lock_(mutex_);

	return processingWaferId_.has_value() ? RecoveryResult::Failed : RecoveryResult::Recovered;
}

std::optional<std::string> SimProcessModule::processingWaferId() const 
{
	std::lock_guard<std::mutex> lock(mutex_);
	return processingWaferId_;
}
