#pragma once

#include <mutex>
#include <vector>
#include <string>
#include <utility>
#include <deque>
#include <cstdint>

#include "equipment_event.h"
#include "config/event_log_config.h"
#include "persistence/ievent_repository.h"


class EventLog
{
private:
	const EventLogConfig config_;
	mutable std::mutex mutex_;
	std::deque<EquipmentEvent> events_;
	std::uint64_t nextSequenceId_ = 1;
	IEventRepository* repository_ = nullptr;


public:
	explicit EventLog(
		EventLogConfig config = {},
		IEventRepository* repository = nullptr
	)
		: config_(config), repository_(repository)
	{
		if (repository_)
		{
			auto history = repository_->getAll();

			for (const auto& event : history)
			{
				events_.push_back(event);
				nextSequenceId_ = event.sequenceId + 1;

				if (config_.maxEvents.has_value())
				{
					while (events_.size() > *config_.maxEvents)
						events_.pop_front();
				}
			}
		}
	}

	void record(EventType type, std::string message) {
		std::lock_guard<std::mutex> lock(mutex_);

		EquipmentEvent event{
			0,
			std::chrono::system_clock::now(),
			type,
			EventSeverity::Info,
			std::move(message)
		};

		if (repository_)
		{
			event.sequenceId = repository_->insert(event);
			nextSequenceId_ = event.sequenceId + 1;
		}
		else
		{
			event.sequenceId = nextSequenceId_++;
		}

		events_.push_back(std::move(event));

		if (config_.maxEvents.has_value()) {
			while (events_.size() > *config_.maxEvents) {
				events_.pop_front();
			}
		}
	}

	std::vector<EquipmentEvent> getEvents() const {
		std::lock_guard<std::mutex> lock(mutex_);
		return { events_.begin(), events_.end() };
	}

	std::vector<EquipmentEvent> getEventsSince(
		std::uint64_t sequenceId
	) const {
		std::lock_guard<std::mutex> lock(mutex_);
		std::vector<EquipmentEvent> result;

		for (const EquipmentEvent& event : events_) 
		{
			if (event.sequenceId > sequenceId) {
				result.push_back(event);
			}
		}

		return result;
	}
};
