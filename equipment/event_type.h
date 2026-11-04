#pragma once

enum class EventType
{
    EquipmentStarted,
    EquipmentIdle,
    EquipmentMoving,
    EquipmentProcessing,
    EquipmentStopping,
    EquipmentFaulted,
    EquipmentRecovering,

    RunRequested,
    RunAccepted,
    RunRejected,
    RunCompleted,
    RunFailed,
    RunCancelled,

    RobotInitializing,
    RobotReady,
    RobotPickStarted,
    RobotPickCompleted,
    RobotPlaceStarted,
    RobotPlaceCompleted,
    RobotMoveFailed,

    WaferRegistered,
    WaferLocationUpdated,
    WaferLocationUnknown,
    WaferTrackingFailed,

    ProcessModuleInitializing,
    ProcessModuleReady,
    ProcessStarted,
    ProcessCompleted,
    ProcessFailed,
    ProcessCancelled,

    RecoveryStarted,
    RecoveryCompleted,
    RecoveryFailed,

    SystemWarning,
    SystemError,
};

namespace MiniFab
{
	std::string_view toString(EventType type);
}


