#include "iprocess_module.h"

class SimProcessModule : public IProcessModule
{
private:
    // What state does the simulator need?
	bool isReady_ = false;

public:
    // implement IProcessModule
	void initialize() override 
	{
		isReady_ = true;
	};
	bool isReady() const override
	{
		return isReady_;
	};
	bool process(const Wafer& wafer, const Recipe& recipe) override;
};
