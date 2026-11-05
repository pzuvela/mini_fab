#pragma once

#include <chrono>
#include <string>
#include <cstdint>

#include "event_type.h"
#include "event_severity.h"


struct EquipmentEvent {
	std::uint64_t sequenceId;
	std::uint64_t runId = 0;
	std::chrono::system_clock::time_point timestamp;
	EventType type;
	EventSeverity severity;
	std::string message;
};
