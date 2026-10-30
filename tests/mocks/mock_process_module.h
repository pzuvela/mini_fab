#include "equipment/process_module/iprocess_module.h"

class MockProcessModule : public IProcessModule
{
private:
   	bool isReady_ = false;
	bool processShouldSucceed_;
	bool processWasCalled_ = false;

public:

	MockProcessModule(bool processShouldSucceed);

	void initialize() override 
	{
		isReady_ = true;
	};
	bool isReady() const override
	{
		return isReady_;
	};
	bool process(const Wafer& wafer, const Recipe& recipe) override;
	bool processWasCalled() const;
};
