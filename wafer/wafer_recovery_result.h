#pragma once

enum class WaferRecoveryResult
{
	Recovered,
	EquipmentBusy,
	WaferNotFound,
	InvalidSource,
	InvalidDestination,
	DestinationOccupied,
	RobotFailed,
	TrackingFailed
};

