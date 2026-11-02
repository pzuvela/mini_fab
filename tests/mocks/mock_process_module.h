#include "equipment/process_module/iprocess_module.h"

class MockProcessModule : public IProcessModule
{
private:
   	bool isReady_ = false;
	bool processShouldSucceed_;
	bool recoveryShouldSucceed_;
	bool processWasCalled_ = false;
	bool recoverWasCalled_ = false;

public:

	MockProcessModule(bool processShouldSucceed, bool recoveryShouldSucceed = true);

	void initialize() override 
	{
		isReady_ = true;
	};
	bool isReady() const override
	{
		return isReady_;
	};
	RecoveryResult recover() override;
	OperationResult process(const Wafer& wafer, const Recipe& recipe, std::stop_token stopToken) override;
	bool processWasCalled() const;
	bool recoverWasCalled() const;
};
