#pragma once

#include <SQLiteCpp/SQLiteCpp.h>
#include <cstdint>
#include <vector>
#include <mutex>
#include <string>

#include "equipment/equipment_event.h"
#include "ievent_repository.h"


class SqliteEventRepository : public IEventRepository
{
private:
	mutable std::mutex mutex_;
	SQLite::Database db_;

	void initializeSchema();

public:
	explicit SqliteEventRepository(
		const std::string& databasePath
	);

	std::uint64_t insert(const EquipmentEvent& event) override;
	std::vector<EquipmentEvent> getAll() const override;
};

