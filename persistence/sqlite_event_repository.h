#pragma once

#include <SQLiteCpp/SQLiteCpp.h>
#include <mutex>
#include <string>

class SqliteEventRepository
{
private:
    mutable std::mutex mutex_;
    SQLite::Database db_;

    void initializeSchema();

public:
    explicit SqliteEventRepository(
        const std::string& databasePath
    );
};

