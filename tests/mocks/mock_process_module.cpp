#include "mock_process_module.h"

MockProcessModule::MockProcessModule(bool processShouldSucceed)
	: processShouldSucceed_(processShouldSucceed)
{
}

bool MockProcessModule::process(const Wafer& wafer, const Recipe& recipe, std::stop_token stopToken)
{
	processWasCalled_ = true;
	return processShouldSucceed_;
}

bool MockProcessModule::processWasCalled() const
{
	return processWasCalled_;
}
