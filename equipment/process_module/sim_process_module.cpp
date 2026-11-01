#include <stdexcept>
#include <iostream>
#include "sim_process_module.h"

OperationResult SimProcessModule::process(const Wafer& wafer, const Recipe& recipe, std::stop_token stopToken)
{
	if (!isReady_) 
	{
		throw std::logic_error(
			"SimProcessModule must be initialized before process can be called."
		);
	}

	std::cout << "Processing wafer " << wafer.id << " using " << recipe.name << " for " << recipe.durationSeconds << " seconds \n";

	return OperationResult::Completed;
}
