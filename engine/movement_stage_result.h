#pragma once

#include <optional>
#include "equipment/operation_result.h"
#include "wafer/location_update_result.h"

struct MovementStageResult
{
    OperationResult movementResult;
    std::optional<LocationUpdateResult> locationUpdateResult;
};
