#include "mock_process_module.h"

MockProcessModule::MockProcessModule(bool processShouldSucceed)
	: processShouldSucceed_(processShouldSucceed)
{
}

OperationResult MockProcessModule::process(const Wafer& wafer, const Recipe& recipe, std::stop_token stopToken)
{
	processWasCalled_ = true;
	return processShouldSucceed_ ? OperationResult::Completed : OperationResult::Failed;
}

bool MockProcessModule::processWasCalled() const
{
	return processWasCalled_;
}
