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

