
#include "sqlite_wafer_repository.h"

#include <stdexcept>
#include <string_view>

namespace
{
    Station parseStation(const std::string& value)
    {
        if (value == "LoadPort") return Station::LoadPort;
        if (value == "Aligner") return Station::Aligner;
        if (value == "ProcessModule") return Station::ProcessModule;
        if (value == "Output") return Station::Output;

        throw std::runtime_error(
            "Unknown station in database: " + value);
    }

    void validateLocation(const Location& location)
    {
        if (location.slot < 1 ||
            toString(location.station) == "Unknown")
        {
            throw std::invalid_argument("Invalid wafer location");
        }
    }
}

SqliteWaferRepository::SqliteWaferRepository(
    const std::string& databasePath)
    : db_(databasePath,
          SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE)
{
    initializeSchema();
}

void SqliteWaferRepository::initializeSchema()
{
    db_.exec(R"(
        CREATE TABLE IF NOT EXISTS wafers (
            id TEXT PRIMARY KEY
                CHECK(length(id) > 0),
            station TEXT NOT NULL
                CHECK(station IN (
                    'LoadPort', 'Aligner',
                    'ProcessModule', 'Output'
                )),
            slot INTEGER NOT NULL CHECK(slot > 0),
            UNIQUE(station, slot)
        );
    )");
}

std::vector<Wafer> SqliteWaferRepository::getAll() const
{
    std::lock_guard lock(mutex_);

    SQLite::Statement query(
        db_,
        "SELECT id, station, slot "
        "FROM wafers ORDER BY id"
    );

    std::vector<Wafer> wafers;

    while (query.executeStep())
    {
        wafers.push_back(Wafer{
            query.getColumn(0).getString(),
            Location{
                parseStation(query.getColumn(1).getString()),
                query.getColumn(2).getInt()
            }
        });
    }

    return wafers;
}

bool SqliteWaferRepository::insert(const Wafer& wafer)
{
    if (wafer.id.empty())
        throw std::invalid_argument("Empty wafer ID");

    validateLocation(wafer.location);

    std::lock_guard lock(mutex_);

    SQLite::Statement query(
        db_,
        "INSERT OR IGNORE INTO wafers "
        "(id, station, slot) VALUES (?, ?, ?)"
    );

    query.bind(1, wafer.id);
    query.bind(2, std::string(toString(wafer.location.station)));
    query.bind(3, wafer.location.slot);

    return query.exec() == 1;
}

bool SqliteWaferRepository::updateLocation(
    const std::string& waferId,
    const Location& location)
{
    validateLocation(location);

    std::lock_guard lock(mutex_);

    SQLite::Statement query(
        db_,
        "UPDATE wafers SET station = ?, slot = ? "
        "WHERE id = ?"
    );

    query.bind(1, std::string(toString(location.station)));
    query.bind(2, location.slot);
    query.bind(3, waferId);

    return query.exec() == 1;
}
