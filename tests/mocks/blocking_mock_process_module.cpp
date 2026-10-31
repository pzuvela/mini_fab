#include <mutex>

#include "blocking_mock_process_module.h"


bool BlockingMockProcessModule::process(const Wafer& wafer, const Recipe& recipe, std::stop_token stopToken)
{
	std::unique_lock<std::mutex> lock(mutex_);

	processingStarted_ = true;

	condition_.notify_all();

	condition_.wait(lock, stopToken, [this]()
	{
		return released_;
	});

	return true;
}

void BlockingMockProcessModule::waitUntilProcessingStarts() 
{
	std::unique_lock<std::mutex> lock(mutex_);

	condition_.wait(lock, [this]()
	{
		return processingStarted_;
	});
}


void BlockingMockProcessModule::release() 
{
	{
		std::lock_guard<std::mutex> lock(mutex_);
		released_ = true;
	}

	condition_.notify_all();
}

void BlockingMockProcessModule::initialize()
{
	ready_ = true;
}

bool BlockingMockProcessModule::isReady() const
{
	return ready_;
}
