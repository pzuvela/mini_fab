#include "mock_process_module.h"

MockProcessModule::MockProcessModule(bool processShouldSucceed, bool recoveryShouldSucceed)
	: processShouldSucceed_(processShouldSucceed), recoveryShouldSucceed_(recoveryShouldSucceed)
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

RecoveryResult MockProcessModule::recover() 
{
	recoverWasCalled_ = true;
	return recoveryShouldSucceed_ ? RecoveryResult::Recovered : RecoveryResult::Failed;
}

bool MockProcessModule::recoverWasCalled() const 
{
	return recoverWasCalled_;
}
