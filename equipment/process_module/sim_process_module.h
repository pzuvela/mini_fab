#include <optional>
#include <condition_variable>
#include <mutex>

#include "iprocess_module.h"

class SimProcessModule : public IProcessModule
{
private:
	bool isReady_ = false;
	std::optional<std::string> processingWaferId_;
	std::condition_variable_any conditionVariable_;
	mutable std::mutex mutex_;
public:
	void initialize() override 
	{
		std::lock_guard<std::mutex> lock(mutex_);
		isReady_ = true;
	};
	bool isReady() const override
	{
		std::lock_guard<std::mutex> lock(mutex_);
		return isReady_;
	};
	RecoveryResult recover() override;
	OperationResult process(
		const Wafer& wafer, 
		const Recipe& recipe, 
		std::stop_token stopToken
	) override;
	std::optional<std::string> processingWaferId() const;
};
