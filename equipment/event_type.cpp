#include <array>
#include <cstddef>
#include <string_view>

#include "event_type.h"

namespace MiniFab
{
    inline constexpr std::array<std::string_view, 35> eventTypeNames{
        "EquipmentStarted",
        "EquipmentIdle",
        "EquipmentMoving",
        "EquipmentProcessing",
        "EquipmentStopping",
        "EquipmentFaulted",
        "EquipmentRecovering",
        "RunRequested",
        "RunAccepted",
        "RunRejected",
        "RunCompleted",
        "RunFailed",
        "RunCancelled",
        "RobotInitializing",
        "RobotReady",
        "RobotPickStarted",
        "RobotPickCompleted",
        "RobotPlaceStarted",
        "RobotPlaceCompleted",
        "RobotMoveFailed",
        "WaferRegistered",
        "WaferLocationUpdated",
        "WaferLocationUnknown",
        "WaferTrackingFailed",
        "ProcessModuleInitializing",
        "ProcessModuleReady",
        "ProcessStarted",
        "ProcessCompleted",
        "ProcessFailed",
        "ProcessCancelled",
        "RecoveryStarted",
        "RecoveryCompleted",
        "RecoveryFailed",
        "SystemWarning",
        "SystemError"
    };

    std::string_view toString(EventType type)
    {
        const auto index = static_cast<std::size_t>(type);

        if (index >= eventTypeNames.size())
            return "Unknown";

        return eventTypeNames[index];
    }
}
