#include <mutex>
#include <thread>
#include <future>

#include "equipment_state.h"
#include "state_transition.h"
#include "wafer/iwafer_manager.h"
#include "equipment/robot/irobot.h"
#include "equipment/process_module/iprocess_module.h"


class ProcessEngine
{
private:
    IWaferManager& mgr_;
    IRobot& robot_;
    IProcessModule& processModule_;
    EquipmentState state_ = EquipmentState::Idle;
    mutable std::mutex mutex_;
    std::jthread worker_;
    std::future<bool> resultFuture_;

    bool run(
        Wafer& wafer,
        const Location& destination
    );

    bool process(
        const Wafer& wafer,
        const Recipe& recipe
    );

    bool executeProcessWafer(
        Wafer& wafer,
        const Location& processModuleLocation,
        const Recipe& recipe,
	std::stop_token stopToken
    );

    void setState(EquipmentState newState);

    bool tryTransition(EquipmentState expectedState, EquipmentState newState);

    Wafer* acceptWaferForProcessing(const std::string& waferId);

public:
    ProcessEngine(
        IWaferManager& waferManager,
        IRobot& robot,
        IProcessModule& processModule
    );

    bool processWafer(
        const std::string& waferId,
        const Location& processModuleLocation,
        const Recipe& recipe
    );

    bool startProcessWafer(
        const std::string& waferId,
        const Location& processModuleLocation,
        const Recipe& recipe
    );

    bool waitForCompletion();

    EquipmentState state() const;

    bool reset();

    StateTransition cancel();
};
