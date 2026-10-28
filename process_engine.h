#include "enums/equipment_state.h"
#include "irobot.h"
#include "iprocess_module.h"


class ProcessEngine
{
private:
    IRobot& robot_;
    IProcessModule& processModule_;
    EquipmentState state_ = EquipmentState::Idle;

public:
    ProcessEngine(
        IRobot& robot,
        IProcessModule& processModule
    );

    bool run(
        Wafer& wafer,
        const Location& destination
    );

    bool process(
        const Wafer& wafer,
        const Recipe& recipe
    );

    bool processWafer(
        Wafer& wafer,
        const Location& processModuleLocation,
        const Recipe& recipe
    );

    EquipmentState state() const;

    bool reset();
};