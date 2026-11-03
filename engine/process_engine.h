#include <mutex>
#include <thread>
#include <future>
#include <optional>

#include "equipment/operation_result.h"
#include "fault_source.h"
#include "movement_stage_result.h"
#include "equipment_state.h"
#include "state_transition.h"
#include "wafer/iwafer_manager.h"
#include "equipment/robot/irobot.h"
#include "equipment/process_module/iprocess_module.h"
#include "wafer/wafer_tracking_recovery_context.h"


class ProcessEngine
{
private:
    IWaferManager& mgr_;
    IRobot& robot_;
    IProcessModule& processModule_;
    EquipmentState state_ = EquipmentState::Idle;
    mutable std::mutex mutex_;
    std::jthread worker_;
    std::future<OperationResult> resultFuture_;
    std::optional<FaultSource> faultSource_;
    std::optional<WaferTrackingRecoveryContext> recoveryContext_;

    MovementStageResult run(
        Wafer& wafer,
        const Location& destination,
	std::stop_token stopToken
    );

    OperationResult process(
        const Wafer& wafer,
        const Recipe& recipe,
	std::stop_token stopToken
    );

    OperationResult executeProcessWafer(
        Wafer& wafer,
        const Location& processModuleLocation,
        const Recipe& recipe,
	std::stop_token stopToken = {}
    );

    OperationResult resultFromFailedTransition(std::stop_token stopToken);

    bool completeMovement();

    OperationResult returnWaferToOriginalLocation(
	Wafer& wafer,
	const Location& originalLocation,
	std::stop_token stopToken 
    );

    OperationResult moveWaferToProcessModule(
	Wafer& wafer,
	const Location& processModuleLocation,
	const Location& originalLocation,
	std::stop_token stopToken
    );

    OperationResult processWaferAtModule(
	Wafer& wafer,
	const Recipe& recipe,
	const Location& location,
	std::stop_token stopToken 
    );

    std::optional<FaultSource> beginRecovery();

    RecoveryResult recoverFaultSource(FaultSource faultSource);

    void completeRecovery(RecoveryResult recoveryResult);

    void setState(EquipmentState newState);

    void setFaulted(FaultSource component);

    bool tryTransition(EquipmentState expectedState, EquipmentState newState);

    Wafer* acceptWaferForProcessing(const std::string& waferId);

public:
    ProcessEngine(
        IWaferManager& waferManager,
        IRobot& robot,
        IProcessModule& processModule
    );

    OperationResult processWafer(
        const std::string& waferId,
        const Location& processModuleLocation,
        const Recipe& recipe
    );

    OperationResult startProcessWafer(
        const std::string& waferId,
        const Location& processModuleLocation,
        const Recipe& recipe
    );

    OperationResult waitForCompletion();

    EquipmentState state() const;

    RecoveryResult reset();

    StateTransition cancel();
};
