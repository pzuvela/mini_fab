#pragma once

#include <SQLiteCpp/SQLiteCpp.h>
#include <mutex>
#include <string>

#include "wafer/iwafer_repository.h"

class SqliteWaferRepository : public IWaferRepository
{
private:
    mutable std::mutex mutex_;
    SQLite::Database db_;

    void initializeSchema();

public:
    explicit SqliteWaferRepository(
        const std::string& databasePath);

    std::vector<Wafer> getAll() const override;

    bool insert(const Wafer& wafer) override;

    bool updateLocation(
        const std::string& waferId,
        const Location& location) override;
};
