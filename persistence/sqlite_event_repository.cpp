#include "sqlite_event_repository.h"

SqliteEventRepository::SqliteEventRepository(
    const std::string& databasePath
)
    : db_(databasePath,
          SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE)
{
    initializeSchema();
}

void SqliteEventRepository::initializeSchema()
{
    db_.exec(R"(
        CREATE TABLE IF NOT EXISTS equipment_events (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp_ms INTEGER NOT NULL,
            type TEXT NOT NULL,
            severity TEXT NOT NULL,
            message TEXT NOT NULL
        );
    )");
}

std::uint64_t SqliteEventRepository::insert(
    const EquipmentEvent& event)
{
    std::lock_guard lock(mutex_);

    SQLite::Statement query(
        db_,
        "INSERT INTO equipment_events "
        "(timestamp_ms, type, severity, message) "
        "VALUES (?, ?, ?, ?)"
    );

    auto timestampMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            event.timestamp.time_since_epoch()
        ).count();

    query.bind(1, static_cast<std::int64_t>(timestampMs));
    query.bind(2, std::string(MiniFab::toString(event.type)));
    query.bind(3, std::string(MiniFab::toString(event.severity)));
    query.bind(4, event.message);

    query.exec();

    return static_cast<std::uint64_t>(db_.getLastInsertRowid());
}

std::vector<EquipmentEvent> SqliteEventRepository::getAll() const
{
    std::lock_guard lock(mutex_);

    SQLite::Statement query(
        db_,
        "SELECT id, timestamp_ms, type, severity, message "
        "FROM equipment_events ORDER BY id"
    );

    std::vector<EquipmentEvent> events;

    while (query.executeStep())
    {
        auto timestampMs = query.getColumn(1).getInt64();

        events.push_back(EquipmentEvent{
            static_cast<std::uint64_t>(
                query.getColumn(0).getInt64()
            ),
            std::chrono::system_clock::time_point{
                std::chrono::milliseconds{timestampMs}
            },
            MiniFab::eventTypeFromString(
                query.getColumn(2).getString()
            ),
            MiniFab::severityFromString(
                query.getColumn(3).getString()
            ),
            query.getColumn(4).getString()
        });
    }

    return events;
}
